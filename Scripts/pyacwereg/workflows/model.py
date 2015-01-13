#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Author: oesteban
# @Date:   2014-10-23 14:43:23
# @Last Modified by:   oesteban
# @Last Modified time: 2015-01-13 13:09:56

import os
import os.path as op

import nipype.pipeline.engine as pe             # pipeline engine
from nipype.interfaces import io as nio              # Data i/o
from nipype.interfaces import utility as niu         # utility
from nipype.interfaces import fsl
from nipype.interfaces import freesurfer as fs
from nipype.algorithms.misc import NormalizeProbabilityMapSet as Normalize

from pysdcev.workflows.smri import extract_surface
from pysdcev.workflows.distortion import bspline_deform

from pyacwereg.interfaces import phantoms as pip
from pyacwereg.interfaces.warps import FieldBasedWarp, InverseField
from pyacwereg.interfaces.utility import Surf2Vol


def generate_phantom(name='PhantomGeneration'):
    """
    A phantom generation workflow
    """
    inputnode = pe.Node(niu.IdentityInterface(
        fields=['shape', 'hi_matrix', 'lo_matrix', 'snr', 'cortex',
                'grid_size']),
        name='inputnode')

    outputnode = pe.Node(niu.IdentityInterface(
        fields=['out_signal', 'out_mask', 'out_tpms', 'out_surfs',
                'out_field', 'out_coeff', 'grid_size']),
        name='outputnode')

    refnode = pe.Node(niu.IdentityInterface(
        fields=['out_signal', 'out_mask', 'out_tpms', 'out_surfs']),
        name='refnode')

    model = pe.Node(pip.Phantom(), name='GenerateModel')
    split = pe.Node(fsl.Split(dimension='t'), name='Split')
    sels0 = pe.Node(niu.Split(splits=[1, 2], squeeze=True),
                    name='SepModel0')
    sels1 = pe.Node(niu.Split(splits=[1, 1, 1], squeeze=True),
                    name='SepModel1')
    signal0 = pe.Node(pip.SimulateSMRI(), name='Simulate0')
    merge0 = pe.Node(niu.Merge(2), name='SimMerge0')

    surf0 = extract_surface('GenSurf0')
    surf0.inputs.inputnode.labels = [1]
    surf0.inputs.inputnode.name = '00.white'
    surf1 = extract_surface('GenSurf1')
    surf1.inputs.inputnode.labels = [1]
    surf1.inputs.inputnode.name = '01.pial'
    msurf = pe.Node(niu.Merge(2), name='MergeSurfs')

    dist = bspline_deform(n_tissues=0)

    norm = pe.Node(Normalize(), name='NormalizeTPMs')

    surf2vol = pe.Node(Surf2Vol(), name='Surf2Volume')

    tpmmsk = pe.Node(niu.Split(splits=[2]), name='TPMsSplit')
    msk = pe.Node(niu.Function(function=_bin_n_msk, input_names=['in_files'],
                               output_names=['out_file']), name='binNmsk')
    sels = pe.Node(niu.Split(splits=[1, 1], squeeze=True),
                   name='SeparateTissue')

    merge1 = pe.Node(niu.Merge(2), name='SimMerge')

    signal1 = pe.Node(pip.SimulateSMRI(), name='Simulate1')

    wf = pe.Workflow(name=name)
    wf.connect([
        (inputnode,   model,       [('shape', 'shape'),
                                    ('hi_matrix', 'matrix_size'),
                                    ('cortex', 'cortex')]),
        (model,       split,       [('out_file', 'in_file')]),
        (split,       sels1,       [('out_files', 'inlist')]),
        (sels1,       signal0,     [('out2', 'frac_wm'),
                                    ('out3', 'frac_gm')]),
        (signal0,     surf0,       [('out_t1w', 'inputnode.norm')]),
        (sels1,       surf0,       [('out2', 'inputnode.aseg')]),
        (signal0,     surf1,       [('out_t1w', 'inputnode.norm')]),
        (model,       surf1,       [('out_mask', 'inputnode.aseg')]),
        (surf0,       msurf,       [('outputnode.out_surf', 'in1')]),
        (surf1,       msurf,       [('outputnode.out_surf', 'in2')]),
        (split,       sels0,       [('out_files', 'inlist')]),
        (inputnode,   dist,        [('grid_size', 'inputnode.grid_size')]),
        (msurf,       dist,        [('out', 'inputnode.in_surfs')]),
        (model,       dist,        [('out_mask', 'inputnode.in_mask')]),
        (sels0,       dist,        [('out2', 'inputnode.in_file')]),

        (signal0,     surf2vol,    [('out_t1w', 'reference')]),
        (dist,        surf2vol,    [('outputnode.out_surfs', 'surfaces')]),
        (surf2vol,    norm,        [('out_tpm', 'in_files')]),
        (norm,        sels,        [('out_files', 'inlist')]),
        (sels,        signal1,     [('out1', 'frac_wm'),
                                    ('out2', 'frac_gm')]),
        (inputnode,   signal1,     [('snr', 'snr')]),
        (signal1,     merge1,      [('out_t1w', 'in1'),
                                    ('out_t2w', 'in2')]),
        (norm,        tpmmsk,      [('out_files', 'inlist')]),
        (tpmmsk,      msk,         [('out1', 'in_files')]),

        # reference outputs
        (inputnode,   outputnode,  [('grid_size', 'grid_size')]),
        (signal0,     merge0,      [('out_t1w', 'in1'),
                                    ('out_t2w', 'in2')]),
        (msurf,       refnode,     [('out', 'out_surfs')]),
        (sels0,       refnode,     [('out2', 'out_tpms')]),
        (model,       refnode,     [('out_mask', 'out_mask')]),
        (merge0,      refnode,     [('out', 'out_signal')]),

        # distorted outputs
        (merge1,      outputnode,  [('out', 'out_signal')]),
        (msk,         outputnode,  [('out_file', 'out_mask')]),
        (tpmmsk,      outputnode,  [('out1', 'out_tpms')]),
        (dist,        outputnode,  [('outputnode.out_field', 'out_field'),
                                    ('outputnode.out_coeff', 'out_coeff'),
                                    ('outputnode.out_surfs', 'out_surfs')])
    ])
    return wf


def _bin_n_msk(in_files):
    import nibabel as nb
    import numpy as np
    import os.path as op

    in_files = np.atleast_1d(in_files).tolist()
    nii = [nb.load(f) for f in in_files]
    data = np.array([im.get_data() for im in nii])
    data = np.sum(data, axis=0)
    msk = np.zeros_like(data, dtype=np.uint8)
    msk[data > 0.0] = 1
    hdr = nii[0].get_header().copy()
    hdr.set_data_dtype(np.uint8)
    hdr.set_data_shape(msk.shape)
    out_file = op.abspath('binarized.nii.gz')
    nb.Nifti1Image(msk, nii[0].get_affine(), hdr).to_filename(out_file)
    return out_file
