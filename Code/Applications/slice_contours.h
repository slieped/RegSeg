// --------------------------------------------------------------------------------------
// File:          slice_contours.h
// Date:          Nov 18, 2014
// Author:        code@oscaresteban.es (Oscar Esteban)
// Version:       1.5.5
// License:       GPLv3 - 29 June 2007
// Short Summary:
// --------------------------------------------------------------------------------------
//
// Copyright (c) 2014, code@oscaresteban.es (Oscar Esteban)
// with Signal Processing Lab 5, EPFL (LTS5-EPFL)
// and Biomedical Image Technology, UPM (BIT-UPM)
// All rights reserved.
//
// This file is part of RegSeg
//
// RegSeg is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RegSeg is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RegSeg.  If not, see <http://www.gnu.org/licenses/>.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#ifndef SLICE_CONTOURS_H_
#define SLICE_CONTOURS_H_

// See: http://www.vtk.org/Wiki/VTK/VTK_6_Migration/Factories_now_require_defines
// See: https://gist.github.com/certik/5687727
#include <vtkVersion.h>

#if VTK_MAJOR_VERSION <= 5
	#include <vtkGraphicsFactory.h>
	#include <vtkImagingFactory.h>
#else
	#include <vtkAutoInit.h>
#endif


#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>

#include <vector>

#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkImageActor.h>
#include <vtkImageProperty.h>
#include <vtkImageMapper3D.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkImageData.h>
#include <vtkSphereSource.h>
#include <vtkMetaImageWriter.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkImageStencil.h>
#include <vtkPointData.h>
#include <vtkCutter.h>
#include <vtkPlane.h>
#include <vtkStripper.h>
#include <vtkLinearExtrusionFilter.h>
#include <vtkImageMapper.h>
#include <vtkImageSliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkPolyDataReader.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkPNGWriter.h>
#include <vtkCamera.h>
#include <vtkCornerAnnotation.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkMatrix4x4.h>
#include <vtkLookupTable.h>
#include <vtkImageMapToColors.h>
#include <vtkImageResliceMapper.h>


#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkRescaleIntensityImageFilter.h>

#include <vtkImageMagnify.h>
#include <vtkImageViewer2.h>
#include <vtkImageReslice.h>

#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleImage.h>
#include <vtkRenderer.h>
#include <vtkWindowToImageFilter.h>
#include <vtkImageMapToWindowLevelColors.h>
#include <vtkAxesActor.h>
#include <vtkCubeAxesActor2D.h>

#include "itkImageToVTKImageFilter.h"

namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;

static const unsigned int DIMENSION = 3;

typedef itk::Image<float, DIMENSION>                         ImageType;
typedef typename ImageType::Pointer                          ImagePointer;
typedef itk::ImageFileReader<ImageType>                      ReaderType;
typedef typename ReaderType::Pointer                         ReaderPointer;
typedef itk::ImageToVTKImageFilter<ImageType>                ConnectorType;
typedef itk::RescaleIntensityImageFilter< ImageType, ImageType > RescaleFilterType;

// Matrices for axial, coronal, sagittal, oblique view orientations
static const double axialElements[16] = {
         1, 0, 0, 0,
         0, 1, 0, 0,
         0, 0, 1, 0,
         0, 0, 0, 1 };

static const double coronalElements[16] = {
         1, 0, 0, 0,
         0, 0, 1, 0,
         0,-1, 0, 0,
         0, 0, 0, 1 };

static const double sagittalElements[16] = {
         0, 0,-1, 0,
         1, 0, 0, 0,
         0,-1, 0, 0,
         0, 0, 0, 1 };

int main(int argc, char *argv[]);

#endif /* SLICE_CONTOURS_H_ */
