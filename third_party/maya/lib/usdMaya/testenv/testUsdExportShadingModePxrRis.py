#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#

import os
import unittest

from pxr import Gf
from pxr import Usd
from pxr import UsdShade

from maya import cmds
from maya import standalone


class testUsdExportShadingModePxrRis(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        cmds.loadPlugin('pxrUsd')

        def openStage(name):
            mayaFile = os.path.abspath(name + '.ma')
            cmds.file(mayaFile, open=True, force=True)
    
            # Export to USD.
            usdFilePath = os.path.abspath(name + '.usda')
            cmds.usdExport(mergeTransformAndShape=True, file=usdFilePath,
                shadingMode='pxrRis')
            
            return Usd.Stage.Open(usdFilePath)
    
        cls._cubeStage = openStage('MarbleCube')
        cls._cubePlanePrismStage = openStage('CubePlanePrism')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testStageOpens(self):
        """
        Tests that the USD stage was opened successfully.
        """
        self.assertTrue(self._cubeStage)
        self.assertTrue(self._cubePlanePrismStage)

    def testExportPxrRisShading(self):
        """
        Tests that exporting a Maya mesh with a simple Maya shading setup
        results in the correct shading on the USD mesh.
        """
        cubePrim = self._cubeStage.GetPrimAtPath('/MarbleCube/Geom/Cube')
        self.assertTrue(cubePrim)

        # Validate the Material prim bound to the Mesh prim.
        material = UsdShade.Material.GetBoundMaterial(cubePrim)
        self.assertTrue(material)
        materialPath = material.GetPath().pathString
        self.assertEqual(materialPath, '/MarbleCube/Looks/MarbleCubeSG')

        # Validate the surface shader that is connected to the material.
        # XXX: Note that the expected number of outputs here is two rather than
        # one, since we are still authoring the UsdRi Bxdf source in addition
        # to the surface terminal for backwards compatibility. When consumers
        # are updated to use the surface terminal instead, this test will have
        # to be updated.
        materialOutputs = material.GetOutputs()
        self.assertEqual(len(materialOutputs), 2)
        materialOutput = material.GetOutput('surface')
        (connectableAPI, outputName, outputType) = materialOutput.GetConnectedSource()
        self.assertEqual(outputName, 'out')
        shader = UsdShade.Shader(connectableAPI)
        self.assertTrue(shader)

        # XXX: Validate the UsdRi Bxdf. This must also be removed when we no
        # longer author it.
        from pxr import UsdRi
        usdRiMaterialAPI = UsdRi.MaterialAPI(material.GetPrim())
        self.assertTrue(usdRiMaterialAPI)
        bxdf = usdRiMaterialAPI.GetBxdf()
        self.assertEqual(bxdf.GetPrim(), shader.GetPrim())

        shaderId = shader.GetIdAttr().Get()
        self.assertEqual(shaderId, 'PxrMayaMarble')

        # Validate the connected input on the surface shader.
        shaderInput = shader.GetInput('placementMatrix')
        self.assertTrue(shaderInput)

        (connectableAPI, outputName, outputType) = shaderInput.GetConnectedSource()
        self.assertEqual(outputName, 'worldInverseMatrix')
        shader = UsdShade.Shader(connectableAPI)
        self.assertTrue(shader)

        shaderId = shader.GetIdAttr().Get()
        self.assertEqual(shaderId, 'PxrMayaPlacement3d')

    def testShaderAttrsAuthoredSparsely(self):
        """
        Tests that only the attributes authored in Maya are exported to USD.
        """
        shaderPrimPath = '/MarbleCube/Looks/MarbleCubeSG/MarbleShader'
        shaderPrim = self._cubeStage.GetPrimAtPath(shaderPrimPath)
        self.assertTrue(shaderPrim)

        shader = UsdShade.Shader(shaderPrim)
        self.assertTrue(shader)

        shaderId = shader.GetIdAttr().Get()
        self.assertEqual(shaderId, 'PxrMayaMarble')

        shaderInputs = shader.GetInputs()
        self.assertEqual(len(shaderInputs), 3)

        inputOutAlpha = shader.GetInput('outAlpha').Get()
        self.assertTrue(Gf.IsClose(inputOutAlpha, 0.0894, 1e-6))

        inputOutColor = shader.GetInput('outColor').Get()
        self.assertTrue(Gf.IsClose(inputOutColor, Gf.Vec3f(0.298, 0.0, 0.0), 1e-6))

        inputPlacementMatrix = shader.GetInput('placementMatrix')
        (connectableAPI, outputName, outputType) = inputPlacementMatrix.GetConnectedSource()
        self.assertEqual(connectableAPI.GetPath().pathString,
            '/MarbleCube/Looks/MarbleCubeSG/MarbleCubePlace3dTexture')

        shaderOutputs = shader.GetOutputs()
        self.assertEqual(len(shaderOutputs), 1)
        
    def testShaderAssignmentWithMergedShapes(self):
        stage = self._cubePlanePrismStage
        # The MarbleCube should NOT be merged, as it has other transforms w/ shapes below it
        cubePrim = stage.GetPrimAtPath('/Geom/MarbleCube/MarbleCubeShape')
        cubeXformPrim = stage.GetPrimAtPath('/Geom/MarbleCube')
        # The other two should be merged
        planePrim = stage.GetPrimAtPath('/Geom/MarbleCube/CheckerPlane')
        prismPrim = stage.GetPrimAtPath('/Geom/MarbleCube/NoTexturePrism')
        
        meshes = (cubePrim, planePrim, prismPrim)
        
        for mesh in meshes:
            self.assertTrue(mesh.IsValid())
            self.assertEqual(mesh.GetTypeName(), 'Mesh')
        self.assertTrue(cubeXformPrim)
        self.assertEqual(cubeXformPrim.GetTypeName(), 'Xform')
        
        # The xform shouldn't have anything assigned to it!    
        xformMat = UsdShade.Material.GetBoundMaterial(cubeXformPrim)
        self.assertFalse(xformMat)
        
        # the cube should have the marble assigned to it...
        marble = UsdShade.Material.GetBoundMaterial(cubePrim)
        self.assertTrue(marble)
        self.assertEqual(marble.GetPath().name, "MarbleCubeSG")

        # the plane should have the checker assigned to it...
        checker = UsdShade.Material.GetBoundMaterial(planePrim)
        self.assertTrue(checker)
        self.assertEqual(checker.GetPath().name, "CheckerPlaneSG")
        
        # ...and the prism shouldn't have a material
        prismMat = UsdShade.Material.GetBoundMaterial(prismPrim)
        self.assertFalse(prismMat)


if __name__ == '__main__':
    unittest.main(verbosity=2)
