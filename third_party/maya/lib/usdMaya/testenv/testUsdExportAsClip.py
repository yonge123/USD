#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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

from pxr import Usd, UsdGeom, Gf


class testUsdExportAsClip(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        cmds.file(os.path.abspath('UsdExportAsClipTest.ma'), open=True,
            force=True)

        cmds.loadPlugin('pxrUsd', quiet=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _ValidateSamples(self, stage, expectedNumSamples):

        print stage.ExportToString()
        cube = stage.GetPrimAtPath('/pCube1')
        self.assertTrue(cube)
        pointsAttr = cube.GetAttribute('points')

        numTimeSamples = pointsAttr.GetNumTimeSamples()
        self.assertTrue(Gf.IsClose(numTimeSamples, expectedNumSamples, 1e-6))

    def testExportAsClip(self):
        # first 5 frames have no animation
        usdFile = os.path.abspath('UsdExportAsClip_cube.001.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile, asClip=True, frameRange=(1, 5))
        stage = Usd.Stage.Open(usdFile)
        self._ValidateSamples(stage, 1)

        # next 5 frames have no animation
        usdFile = os.path.abspath('UsdExportAsClip_cube.005.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile, asClip=True, frameRange=(5, 10))
        stage = Usd.Stage.Open(usdFile)
        self._ValidateSamples(stage, 1)

        # next 5 frames have deformation animation
        usdFile = os.path.abspath('UsdExportAsClip_cube.010.usda')
        frames = (10, 15)
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile, asClip=True, frameRange=frames)
        stage = Usd.Stage.Open(usdFile)
        self._ValidateSamples(stage, frames[1] + 1 - frames[0])

        # next 5 frames have no animation
        usdFile = os.path.abspath('UsdExportAsClip_cube.015.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile, asClip=True, frameRange=(15, 20))
        stage = Usd.Stage.Open(usdFile)
        self._ValidateSamples(stage, 1)

        # test that non clip mode is different:
        usdFile = os.path.abspath('UsdExportAsClip_cubeNonClip.001.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile, asClip=False, frameRange=(1, 5))
        stage = Usd.Stage.Open(usdFile)
        self._ValidateSamples(stage, 0)

if __name__ == '__main__':
    unittest.main(verbosity=2)
