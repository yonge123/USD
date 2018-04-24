#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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

from maya import cmds
from maya import standalone

from pxr import Usd
from pxr import UsdGeom
from pxr import Vt
from pxr import Gf

class testUsdExportRoot(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        cmds.file(os.path.abspath('UsdExportRootTest.ma'), open=True,
            force=True)

        cmds.loadPlugin('pxrUsd', quiet=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()
        
    def doExportImportOneMethod(self, method, shouldError=False, root=None, selection=None):
        name = 'UsdExportRoot_root-{root}_sel-{sel}'.format(root=root, sel=selection)
        if method == 'cmd':
            name += '.usda'
        elif method == 'translator':
            # the translator seems to be only able to export .usd - or at least,
            # I couldn't find an option to do .usda instead...
            name += '.usd'
        usdFile = os.path.abspath(name)
        
        if method == 'cmd':
            exportMethod = cmds.usdExport
            args = ()
            kwargs = {
                'file': usdFile,
                'mergeTransformAndShape': True,
                'shadingMode': 'none',
            }
            if root:
                kwargs['root'] = root
            if selection:
                kwargs['selection'] = 1
                cmds.select(selection)
        else:
            exportMethod = cmds.file
            args = (usdFile,)
            kwargs = {
                'type': 'pxrUsdExport',
                'f': 1,
            }
            if root:
                kwargs['options'] = 'root={}'.format(root)
            if selection:
                kwargs['exportSelected'] = 1
            else:
                kwargs['exportAll'] = 1

        if shouldError:
            self.assertRaises(RuntimeError, exportMethod, *args, **kwargs)
            return
        exportMethod(*args, **kwargs)
        return Usd.Stage.Open(usdFile)
        
    def doExportImportTest(self, validator, shouldError=False, root=None, selection=None):
        stage = self.doExportImportOneMethod('cmd', shouldError=shouldError,
                                             root=root, selection=selection)
        validator(stage)
        stage = self.doExportImportOneMethod('translator', shouldError=shouldError,
                                             root=root, selection=selection)
        validator(stage)

    def testNonOverlappingSelectionRoot(self):
        # test that the command errors if we give it a root + selection that
        # have no intersection
        def validator(stage):
            pass
        self.doExportImportTest(validator, shouldError=True, root='Mid', selection='OtherTop')

    def testExportRoot_rootTop_selNone(self):
        def validator(stage):
            self.assertTrue(UsdGeom.Mesh.Get(stage, '/Top/Mid/Cube').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherTop').GetPrim().IsValid())
            self.assertTrue(UsdGeom.Mesh.Get(stage, '/Top/OtherMid').GetPrim().IsValid())
            self.assertTrue(UsdGeom.Mesh.Get(stage, '/Top/Mid/OtherLowest').GetPrim().IsValid())
        self.doExportImportTest(validator, root='Top')

    def testExportRoot_rootTop_selTop(self):
        def validator(stage):
            self.assertTrue(UsdGeom.Mesh.Get(stage, '/Top/Mid/Cube').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherTop').GetPrim().IsValid())
            self.assertTrue(UsdGeom.Mesh.Get(stage, '/Top/OtherMid').GetPrim().IsValid())
            self.assertTrue(UsdGeom.Mesh.Get(stage, '/Top/Mid/OtherLowest').GetPrim().IsValid())
        self.doExportImportTest(validator, root='Top', selection='Top')

    def testExportRoot_rootTop_selMid(self):
        def validator(stage):
            self.assertTrue(UsdGeom.Mesh.Get(stage, '/Top/Mid/Cube').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherTop').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/Top/OtherMid').GetPrim().IsValid())
            self.assertTrue(UsdGeom.Mesh.Get(stage, '/Top/Mid/OtherLowest').GetPrim().IsValid())
        self.doExportImportTest(validator, root='Top', selection='Mid')

    def testExportRoot_rootTop_selCube(self):
        def validator(stage):
            self.assertTrue(UsdGeom.Mesh.Get(stage, '/Top/Mid/Cube').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherTop').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/Top/OtherMid').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/Top/Mid/OtherLowest').GetPrim().IsValid())
        self.doExportImportTest(validator, root='Top', selection='Cube')

    def testExportRoot_rootMid_selNone(self):
        def validator(stage):
            self.assertTrue(UsdGeom.Mesh.Get(stage, '/Mid/Cube').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/Top').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherTop').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherMid').GetPrim().IsValid())
            self.assertTrue(UsdGeom.Mesh.Get(stage, '/Mid/OtherLowest').GetPrim().IsValid())
        self.doExportImportTest(validator, root='Mid')

    def testExportRoot_rootMid_selTop(self):
        def validator(stage):
            self.assertTrue(UsdGeom.Mesh.Get(stage, '/Mid/Cube').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/Top').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherTop').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherMid').GetPrim().IsValid())
            self.assertTrue(UsdGeom.Mesh.Get(stage, '/Mid/OtherLowest').GetPrim().IsValid())
        self.doExportImportTest(validator, root='Mid', selection='Top')

    def testExportRoot_rootMid_selMid(self):
        def validator(stage):
            self.assertTrue(UsdGeom.Mesh.Get(stage, '/Mid/Cube').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/Top').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherTop').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherMid').GetPrim().IsValid())
            self.assertTrue(UsdGeom.Mesh.Get(stage, '/Mid/OtherLowest').GetPrim().IsValid())
        self.doExportImportTest(validator, root='Mid', selection='Mid')

    def testExportRoot_rootMid_selCube(self):
        def validator(stage):
            self.assertTrue(UsdGeom.Mesh.Get(stage, '/Mid/Cube').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/Top').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherTop').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherMid').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/Mid/OtherLowest').GetPrim().IsValid())
        self.doExportImportTest(validator, root='Mid', selection='Cube')

    def testExportRoot_rootCube_selNone(self):
        def validator(stage):
            self.assertTrue(UsdGeom.Mesh.Get(stage, '/Cube').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/Top').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/Mid').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherTop').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherMid').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherLowest').GetPrim().IsValid())
        self.doExportImportTest(validator, root='Cube')

    def testExportRoot_rootCube_selTop(self):
        def validator(stage):
            self.assertTrue(UsdGeom.Mesh.Get(stage, '/Cube').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/Top').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/Mid').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherTop').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherMid').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherLowest').GetPrim().IsValid())
        self.doExportImportTest(validator, root='Cube', selection='Top')

    def testExportRoot_rootCube_selMid(self):
        def validator(stage):
            self.assertTrue(UsdGeom.Mesh.Get(stage, '/Cube').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/Top').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/Mid').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherTop').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherMid').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherLowest').GetPrim().IsValid())
        self.doExportImportTest(validator, root='Cube', selection='Mid')

    def testExportRoot_rootCube_selCube(self):
        def validator(stage):
            self.assertTrue(UsdGeom.Mesh.Get(stage, '/Cube').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/Top').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/Mid').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherTop').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherMid').GetPrim().IsValid())
            self.assertFalse(UsdGeom.Mesh.Get(stage, '/OtherLowest').GetPrim().IsValid())
        self.doExportImportTest(validator, root='Cube', selection='Cube')


if __name__ == '__main__':
    unittest.main(verbosity=2)
