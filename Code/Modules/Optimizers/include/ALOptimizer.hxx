// --------------------------------------------------------------------------------------
// File:          ALOptimizer.hxx
// Date:          Jun 24, 2013
// Author:        code@oscaresteban.es (Oscar Esteban)
// Version:       1.0 beta
// License:       GPLv3 - 29 June 2007
// Short Summary:
// --------------------------------------------------------------------------------------
//
// Copyright (c) 2013, code@oscaresteban.es (Oscar Esteban)
// with Signal Processing Lab 5, EPFL (LTS5-EPFL)
// and Biomedical Image Technology, UPM (BIT-UPM)
// All rights reserved.
//
// This file is part of ACWE-Reg
//
// ACWE-Reg is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// ACWE-Reg is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with ACWE-Reg.  If not, see <http://www.gnu.org/licenses/>.
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

#ifndef ALOPTIMIZER_HXX_
#define ALOPTIMIZER_HXX_

#include "ALOptimizer.h"
#include <vector>
#include <vnl/vnl_math.h>
#include <vnl/vnl_matrix.h>
#include <vnl/vnl_diag_matrix.h>
#include <cmath>
#include <itkComplexToRealImageFilter.h>
#include "DisplacementFieldFileWriter.h"
#include <itkMeshFileWriter.h>
#include <sstream>
#include <iostream>
#include <iomanip>
using namespace std;

namespace rstk {

/**
 * Default constructor
 */
template< typename TLevelSetsFunction >
ALOptimizer<TLevelSetsFunction>::ALOptimizer() {
	this->m_LearningRate = itk::NumericTraits<InternalComputationValueType>::One;
	this->m_MaximumStepSizeInPhysicalUnits = itk::NumericTraits<InternalComputationValueType>::Zero;
	this->m_MinimumConvergenceValue = 1e-8;
	this->m_ConvergenceWindowSize = 30;
	this->m_StepSize =  1e-3;
	this->m_A.SetIdentity();
	this->m_A(0,0) = 2.0;
	this->m_A(1,1) = 2.0;
	this->m_A(2,2) = 2.0;
	this->m_B.SetIdentity();
	this->m_B(0,0) = 2.0;
	this->m_B(1,1) = 2.0;
	this->m_B(2,2) = 2.0;
}

template< typename TLevelSetsFunction >
void ALOptimizer<TLevelSetsFunction>
::PrintSelf(std::ostream &os, itk::Indent indent) const {
	Superclass::PrintSelf(os,indent);
	os << indent << "Learning rate:" << this->m_LearningRate << std::endl;
	os << indent << "MaximumStepSizeInPhysicalUnits: "
             << this->m_MaximumStepSizeInPhysicalUnits << std::endl;
}



template< typename TLevelSetsFunction >
void ALOptimizer<TLevelSetsFunction>::Start() {
	itkDebugMacro("ALOptimizer::Start()");

	VectorType zerov = itk::NumericTraits<VectorType>::Zero;
	/* Settings validation */
	if ( this->m_LevelSetsFunction.IsNull() ) {
		itkExceptionMacro("LevelSets Object must be set");
	}

	if (this->m_uField.IsNull()) {
		PointType orig = this->m_LevelSetsFunction->GetReferenceImage()->GetOrigin();
		PointType end;

		typename DeformationFieldType::IndexType end_idx;
		typename DeformationFieldType::SizeType refSize = this->m_LevelSetsFunction->GetReferenceImage()->GetLargestPossibleRegion().GetSize();
		for( size_t dim = 0; dim < DeformationFieldType::ImageDimension; dim++ ) end_idx[dim] = refSize[dim];
		this->m_LevelSetsFunction->GetReferenceImage()->TransformIndexToPhysicalPoint( end_idx, end );
		this->InitializeFields( orig, end, this->m_LevelSetsFunction->GetReferenceImage()->GetDirection() );
	}


	/* Initialize convergence checker */
	this->m_ConvergenceMonitoring = ConvergenceMonitoringType::New();
	this->m_ConvergenceMonitoring->SetWindowSize( this->m_ConvergenceWindowSize );

	/* Must call the superclass version for basic validation and setup */
	Superclass::Start();

//	if( this->m_ReturnBestParametersAndValue )	{
//		this->m_BestParameters = this->GetCurrentPosition( );
//		this->m_CurrentBestValue = NumericTraits< MeasureType >::max();
//	}

	this->m_LevelSetsFunction->CopyInformation( this->m_uField );
	this->m_LevelSetsFunction->Initialize();

	this->m_CurrentIteration = 0;
	this->Resume();
}

template< typename TLevelSetsFunction >
void ALOptimizer<TLevelSetsFunction>::Stop() {

//	if( this->m_ReturnBestParametersAndValue )	{
//		this->GetMetric()->SetParameters( this->m_BestParameters );
//		this->m_CurrentLevelSetsValue = this->m_CurrentBestValue;
//	}
	Superclass::Stop();
}

template< typename TLevelSetsFunction >
void ALOptimizer<TLevelSetsFunction>::Resume() {
	this->m_StopConditionDescription.str("");
	this->m_StopConditionDescription << this->GetNameOfClass() << ": ";
	this->InvokeEvent( itk::StartEvent() );
	this->m_Stop = false;


	while( ! this->m_Stop )	{
		/* Compute metric value/derivative. */
		try	{
			this->m_LevelSetsFunction->ComputeGradient();
#ifndef NDEBUG
			typedef rstk::DisplacementFieldFileWriter<DeformationFieldType> Writer;
			typename Writer::Pointer p = Writer::New();
			std::stringstream ss2;
			ss2 << "gradient_" << std::setfill('0')  << std::setw(3) << this->m_CurrentIteration << ".nii.gz";
			p->SetFileName( ss2.str().c_str() );
			p->SetInput( this->m_LevelSetsFunction->GetGradientMap() );
			p->Update();
#endif
		}
		catch ( itk::ExceptionObject & err ) {
			this->m_StopCondition = Superclass::COSTFUNCTION_ERROR;
			this->m_StopConditionDescription << "LevelSets error during optimization";
			this->Stop();
			throw err;  // Pass exception to caller
		}

		/* Check if optimization has been stopped externally.
		 * (Presumably this could happen from a multi-threaded client app?) */
		if ( this->m_Stop ) {
			this->m_StopConditionDescription << "Stop() called externally";
			break;
		}


		/* Advance one step along the gradient.
		 * This will modify the gradient and update the transform. */
		this->Iterate();

		this->ComputeIterationChange();

#ifndef NDEBUG
			typedef rstk::DisplacementFieldFileWriter<DeformationFieldType> Writer;
			typename Writer::Pointer p = Writer::New();
			std::stringstream ss2;
			ss2 << "field_" << std::setfill('0')  << std::setw(3) << this->m_CurrentIteration << ".nii.gz";
			p->SetFileName( ss2.str().c_str() );
			p->SetInput( this->m_uFieldNext );
			p->Update();
#endif

		/* Update the level sets contour and deformation field */
		double updateNorm = this->m_LevelSetsFunction->UpdateContour( this->m_uFieldNext );

		const VectorType* next = this->m_uFieldNext->GetBufferPointer();
		VectorType* curr = this->m_uField->GetBufferPointer();
		size_t nPix = this->m_uField->GetLargestPossibleRegion().GetNumberOfPixels();

		for( size_t pix = 0; pix<nPix; pix++){
			*(curr+pix) = *(next+pix);
		}


		/* TODO Store best value and position */
		//if ( this->m_ReturnBestParametersAndValue && this->m_CurrentLevelSetsValue < this->m_CurrentBestValue )
		//{
		//	this->m_CurrentBestValue = this->m_CurrentLevelSetsValue;
		//	this->m_BestParameters = this->GetCurrentPosition( );
		//}

#ifndef NDEBUG
			typedef itk::MeshFileWriter< ContourType >     MeshWriterType;
			typename MeshWriterType::Pointer w = MeshWriterType::New();
			size_t nContours =this->m_LevelSetsFunction->GetCurrentContourPosition().size();

			for ( size_t c = 0; c < nContours; c++ ) {
				w->SetInput( this->m_LevelSetsFunction->GetCurrentContourPosition()[c] );
				std::stringstream ss;
				ss << "contour_" << std::setfill('0') << std::setw(2) << c <<"_it_" << std::setw(3) << this->m_CurrentIteration << ".vtk";
				w->SetFileName( ss.str() );
				w->Update();
			}

			ss2.str("");
			ss2 << "field_" << std::setfill('0')  << std::setw(3) << this->m_CurrentIteration << ".nii.gz";
			p->SetFileName( ss2.str().c_str() );
			p->SetInput( this->m_uField );
			p->Update();


			typedef itk::ImageFileWriter< typename TLevelSetsFunction::ROIType > ROIWriter;
			for( size_t r = 0; r <= nContours; r++){
				std::stringstream ss3;
				ss3 << "roi_tfd_it" << std::setfill('0')<<std::setw(3) << this->m_CurrentIteration << "_id" << r << ".nii.gz";
				typename ROIWriter::Pointer wr = ROIWriter::New();
				wr->SetInput( this->m_LevelSetsFunction->GetCurrentRegion(r));
				wr->SetFileName(ss3.str().c_str() );
				wr->Update();
			}
#endif

		std::cout << "[" << this->m_CurrentIteration << "] " << this->m_CurrentLevelSetsValue << " | " << updateNorm << " | " << this->m_LevelSetsFunction->GetValue() << std::endl;

		/*
		 * Check the convergence by WindowConvergenceMonitoringFunction.
		 */

		/*
		this->m_ConvergenceMonitoring->AddEnergyValue( this->m_CurrentLevelSetsValue );
		try {
			this->m_ConvergenceValue = this->m_ConvergenceMonitoring->GetConvergenceValue();
			if (this->m_ConvergenceValue <= this->m_MinimumConvergenceValue) {
				this->m_StopConditionDescription << "Convergence checker passed at iteration " << this->m_CurrentIteration << ".";
				this->m_StopCondition = Superclass::CONVERGENCE_CHECKER_PASSED;
				this->Stop();
				break;
			}
		}
		catch(std::exception & e) {
			std::cerr << "GetConvergenceValue() failed with exception: " << e.what() << std::endl;
		}


		if( this->m_CurrentLevelSetsValue < this->m_ConvergenceValue ) {
			this->m_StopConditionDescription << "Deformation field changed below the minimum threshold.";
			this->m_StopCondition = Superclass::STEP_TOO_SMALL;
			this->Stop();
			break;
		}
		*/

		/* Update and check iteration count */
		this->m_CurrentIteration++;

		if ( this->m_CurrentIteration >= this->m_NumberOfIterations ) {
			this->m_StopConditionDescription << "Maximum number of iterations (" << this->m_NumberOfIterations << ") exceeded.";
			this->m_StopCondition = Superclass::MAXIMUM_NUMBER_OF_ITERATIONS;
			this->Stop();
			break;
		}
	} //while (!m_Stop)
}


template< typename TLevelSetsFunction >
void ALOptimizer<TLevelSetsFunction>::Iterate() {
	itkDebugMacro("Optimizer Iteration");
	double min_val = 1.0e-5;

	DeformationFieldConstPointer shapeGradient = this->m_LevelSetsFunction->GetGradientMap();

#ifndef NDEBUG
	typedef rstk::DisplacementFieldFileWriter<DeformationFieldType> Writer;
	typename Writer::Pointer p = Writer::New();
	std::stringstream ss2;
	ss2 << "gradient2_" << std::setfill('0')  << std::setw(3) << this->m_CurrentIteration << ".nii.gz";
	p->SetFileName( ss2.str().c_str() );
	p->SetInput( shapeGradient );
	p->Update();
#endif

	// Get deformation fields pointers
	const VectorType* dFieldBuffer = this->m_uField->GetBufferPointer();
	const VectorType* sFieldBuffer = shapeGradient->GetBufferPointer();

	// Create container for deformation field components
	DeformationComponentPointer fieldComponent = DeformationComponentType::New();
	fieldComponent->SetDirection( this->m_uField->GetDirection() );
	fieldComponent->SetRegions( this->m_uField->GetLargestPossibleRegion() );
	fieldComponent->Allocate();
	PointValueType* dCompBuffer = fieldComponent->GetBufferPointer();

	ComplexFieldPointer fftNum;

	size_t nPix = fieldComponent->GetLargestPossibleRegion().GetNumberOfPixels();
	for( size_t d = 0; d < Dimension; d++ ) {
		// Get component from vector image
		size_t comp, idx2;
		PointValueType u,grad;
		for(size_t pix = 0; pix< nPix; pix++) {
			u = (*(dFieldBuffer+pix))[d];
			grad = (*(sFieldBuffer+pix))[d];
			*(dCompBuffer+pix) = ( u / this->m_StepSize ) - grad; // ATTENTION: the + symbol depends on the definition of the normal
		}                                                         // OUTWARDS-> + ; INWARDS-> -.
#ifndef NDEBUG
		typedef itk::ImageFileWriter<DeformationComponentType> ComponentWriter;
		typename ComponentWriter::Pointer p = ComponentWriter::New();
		ss2.str("");
		ss2 << "gradient3_" << std::setfill('0')  << std::setw(3) << this->m_CurrentIteration << "_dim" << d << ".nii.gz";
		p->SetFileName( ss2.str().c_str() );
		p->SetInput( fieldComponent );
		p->Update();
#endif

		FFTPointer fftFilter = FFTType::New();
		fftFilter->SetInput( fieldComponent );
		fftFilter->Update();  // This is required for computing the denominator for first time
		FTDomainPointer fftComponent =  fftFilter->GetOutput();

		if ( fftNum.IsNull() ) {
			fftNum = ComplexFieldType::New();
			fftNum->SetRegions( fftComponent->GetLargestPossibleRegion() );
			fftNum->Allocate();
			fftNum->FillBuffer( ComplexType( itk::NumericTraits<ComplexValueType>::Zero, itk::NumericTraits<ComplexValueType>::Zero) );
		}

		// Fill in ND fourier transform of numerator
		ComplexFieldValue* fftBuffer = fftNum->GetBufferPointer();
		ComplexType* fftCompBuffer = fftComponent->GetBufferPointer();
		size_t nFrequel = fftComponent->GetLargestPossibleRegion().GetNumberOfPixels();
		ComplexFieldValue cur;
		for(size_t fr = 0; fr< nFrequel; fr++) {
			cur = *(fftBuffer+fr);
			cur[d] = *(fftCompBuffer+fr);
			*(fftBuffer+fr) = cur;
		}
	}

	this->ApplyRegularizationTerm( fftNum );


	VectorType* nextFieldBuffer = this->m_uFieldNext->GetBufferPointer();
	for( size_t d = 0; d < Dimension; d++ ) {
		// Take out fourier component
		FTDomainPointer fftComponent = FTDomainType::New();
		fftComponent->SetRegions( fftNum->GetLargestPossibleRegion() );
		fftComponent->SetOrigin( fftNum->GetOrigin() );
		fftComponent->SetSpacing( fftNum->GetSpacing() );
		fftComponent->Allocate();
		fftComponent->FillBuffer( ComplexType(0.0,0.0) );
		ComplexType* fftCompBuffer = fftComponent->GetBufferPointer();
		ComplexFieldValue* fftBuffer = fftNum->GetBufferPointer();
		size_t nFrequel = fftComponent->GetLargestPossibleRegion().GetNumberOfPixels();
		for(size_t fr = 0; fr< nFrequel; fr++) {
			*(fftCompBuffer+fr) = (*(fftBuffer+fr))[d];
		}


		IFFTPointer ifft = IFFTType::New();
		ifft->SetInput( fftComponent );

		try {
			ifft->Update();
		}
		catch ( itk::ExceptionObject & err ) {
			this->m_StopCondition = Superclass::UPDATE_PARAMETERS_ERROR;
			this->m_StopConditionDescription << "UpdateDeformationField error";
			this->Stop();
			// Pass exception to caller
			throw err;
		}

		// Set component on this->m_uFieldNext
		const PointValueType* resultBuffer = ifft->GetOutput()->GetBufferPointer();
		PointValueType val;
		for(size_t pix = 0; pix< nPix; pix++) {
			val = *(resultBuffer+pix);
			(*(nextFieldBuffer+pix))[d] = ( fabs(val) > min_val )?val:0.0;
		}

#ifndef NDEBUG
		typedef itk::ImageFileWriter<DeformationComponentType> ComponentWriter;
		typename ComponentWriter::Pointer p = ComponentWriter::New();
		ss2.str("");
		ss2 << "fieldfiltered_" << std::setfill('0')  << std::setw(3) << this->m_CurrentIteration << "_dim" << d << ".nii.gz";
		p->SetFileName( ss2.str().c_str() );
		p->SetInput( ifft->GetOutput() );
		p->Update();
#endif
	}

	this->InvokeEvent( itk::IterationEvent() );
}

template< typename TLevelSetsFunction >
void ALOptimizer<TLevelSetsFunction>
::ApplyRegularizationTerm( typename ALOptimizer<TLevelSetsFunction>::ComplexFieldType* reference ){
	if( this->m_Denominator.IsNull() ) { // If executed for first time, cache the map
		this->InitializeDenominator( reference );
	}

	// Fill reference buffer with elements updated with the filter
	ComplexFieldValue* nBuffer = reference->GetBufferPointer();
	MatrixType* dBuffer = this->m_Denominator->GetBufferPointer();
	size_t nPix = this->m_Denominator->GetLargestPossibleRegion().GetNumberOfPixels();
	ComplexFieldValue curval;
	VectorType realval;
	VectorType imagval;
	MatrixType regTensor;

	for (size_t pix = 0; pix < nPix; pix++ ) {
		curval = *(nBuffer+pix);
		regTensor = *(dBuffer+pix);

		for( size_t i = 0; i<Dimension; i++ ) {
			realval[i] = curval[i].real();
			imagval[i] = curval[i].imag();
		}

		realval = regTensor * realval;
		imagval = regTensor * imagval;

		for( size_t i = 0; i<Dimension; i++ ) {
			curval[i] = ComplexType(realval[i], imagval[i]);
		}


		*(nBuffer+pix) = curval;
	}
}

template< typename TLevelSetsFunction >
void ALOptimizer<TLevelSetsFunction>
::InitializeDenominator( typename ALOptimizer<TLevelSetsFunction>::ComplexFieldType* reference ){
	double pi2 = 2* vnl_math::pi;
	MatrixType I, t;
	I.SetIdentity();
	MatrixType C = ( I * (1.0/this->m_StepSize) + I * m_A ) * pi2;


	this->m_Denominator = TensorFieldType::New();
	this->m_Denominator->SetSpacing(   reference->GetSpacing() );
	this->m_Denominator->SetDirection( reference->GetDirection() );
	this->m_Denominator->SetRegions( reference->GetLargestPossibleRegion().GetSize() );
	this->m_Denominator->Allocate();
	this->m_Denominator->FillBuffer( I );

	typename TensorFieldType::SizeType size = this->m_Denominator->GetLargestPossibleRegion().GetSize();
	MatrixType* buffer = this->m_Denominator->GetBufferPointer();
	size_t nPix = this->m_Denominator->GetLargestPossibleRegion().GetNumberOfPixels();

	// Fill Buffer with denominator data
	double lag_el; // accumulates the FT{lagrange operator}.
	typename TensorFieldType::IndexType idx;
	for (size_t pix = 0; pix < nPix; pix++ ) {
		lag_el = 0.0;
		idx = this->m_Denominator->ComputeIndex( pix );
		for(size_t d = 0; d < Dimension; d++ ) {
			lag_el+= 2.0 * cos( (pi2*idx[d])/size[d]) - 2.0;
		}
		t = C + m_B * lag_el * (-1.0);
		*(buffer+pix) = t.GetInverse();
	}
}

template< typename TLevelSetsFunction >
void ALOptimizer<TLevelSetsFunction>::ComputeIterationChange() {
	VectorType* fnextBuffer = this->m_uFieldNext->GetBufferPointer();
	VectorType* fBuffer = this->m_uField->GetBufferPointer();
	size_t nPix = this->m_uFieldNext->GetLargestPossibleRegion().GetNumberOfPixels();
	double totalNorm = 0;
	VectorType t0,t1;
	for (size_t pix = 0; pix < nPix; pix++ ) {
		t0 = *(fBuffer+pix);
		t1 = *(fnextBuffer+pix);
		totalNorm += ( t1 - t0 ).GetNorm();
	}

	this->m_CurrentLevelSetsValue = totalNorm/nPix;
}


template< typename TLevelSetsFunction >
void
ALOptimizer<TLevelSetsFunction>::InitializeFields(
	 const typename ALOptimizer<TLevelSetsFunction>::PointType orig,
	 const typename ALOptimizer<TLevelSetsFunction>::PointType end,
	 const typename ALOptimizer<TLevelSetsFunction>::DeformationFieldDirectionType dir)
{
	this->m_uField = DeformationFieldType::New();

	typename DeformationFieldType::SpacingType spacing;
	for( size_t dim = 0; dim < DeformationFieldType::ImageDimension; dim++ ) {
		spacing[dim] = fabs( (end[dim]-orig[dim])/(1.0*this->m_GridSize[dim]));
	}

	VectorType zerov;
	zerov.Fill(0.0);

	this->m_uField->SetRegions( this->m_GridSize );
	this->m_uField->SetSpacing( spacing );
	this->m_uField->SetDirection( dir );
	this->m_uField->SetOrigin( orig );
	this->m_uField->Allocate();
	this->m_uField->FillBuffer( zerov );

	this->m_uFieldNext = DeformationFieldType::New();
	this->m_uFieldNext->SetRegions( this->m_uField->GetLargestPossibleRegion() );
	this->m_uFieldNext->SetSpacing( this->m_uField->GetSpacing() );
	this->m_uFieldNext->SetDirection( this->m_uField->GetDirection() );
	this->m_uFieldNext->SetOrigin( this->m_uField->GetOrigin() );
	this->m_uFieldNext->Allocate();
	this->m_uFieldNext->FillBuffer( zerov );
}

}


#endif /* ALOPTIMIZER_HXX_ */