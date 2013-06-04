// --------------------------------------------------------------------------
// File:             MahalanobisLevelSets.hxx
// Date:             27/10/2012
// Author:           code@oscaresteban.es (Oscar Esteban, OE)
// Version:          0.1
// License:          BSD
// --------------------------------------------------------------------------
//
// Copyright (c) 2012, code@oscaresteban.es (Oscar Esteban)
// with Signal Processing Lab 5, EPFL (LTS5-EPFL)
// and Biomedical Image Technology, UPM (BIT-UPM)
// All rights reserved.
// 
// This file is part of ACWERegistration-Debug@Debug
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the names of the LTS5-EFPL and the BIT-UPM, nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY Oscar Esteban ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL OSCAR ESTEBAN BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef MAHALANOBISLEVELSETS_HXX_
#define MAHALANOBISLEVELSETS_HXX_

#include "MahalanobisLevelSets.h"
#include <vnl/vnl_matrix.h>
#include <vnl/vnl_diag_matrix.h>
#include <vnl/algo/vnl_symmetric_eigensystem.h>
#include <vnl/algo/vnl_ldl_cholesky.h>

#include "DisplacementFieldFileWriter.h"
#include <itkMeshFileWriter.h>

namespace rstk {
template <typename TReferenceImageType, typename TCoordRepType>
MahalanobisLevelSets<TReferenceImageType,TCoordRepType>
::MahalanobisLevelSets() {
	this->m_Interp = InterpolatorType::New();

}

template <typename TReferenceImageType, typename TCoordRepType>
void
MahalanobisLevelSets<TReferenceImageType,TCoordRepType>
::PrintSelf( std::ostream& os, itk::Indent indent) const {
	Superclass::PrintSelf(os, indent);

	// TODO PrintSelf parameters
}

template <typename TReferenceImageType, typename TCoordRepType>
void
MahalanobisLevelSets<TReferenceImageType,TCoordRepType>
::InitializeSamplingGrid() {
	this->m_ReferenceSamplingGrid = DeformationFieldType::New();
	typename ReferenceImageType::SpacingType sp = this->m_ReferenceImage->GetSpacing();
	double spacing = itk::NumericTraits< double >::max();

	for (size_t i = 0; i<Dimension; i++ ){
		if (sp[i] < spacing )
			spacing = sp[i];
	}

	sp.Fill( spacing * 0.25 );

	typename ReferenceImageType::PointType origin = this->m_ReferenceImage->GetOrigin();
	typename ReferenceImageType::SizeType size = this->m_ReferenceImage->GetLargestPossibleRegion().GetSize();
	typename itk::ContinuousIndex<double, Dimension> idx;
	for (size_t i = 0; i<Dimension; i++ ){
		idx[i]=size[i]-1.0;
	}

	typename ReferenceImageType::PointType end;
	this->m_ReferenceImage->TransformContinuousIndexToPhysicalPoint( idx, end );

	for (size_t i = 0; i<Dimension; i++ ){
		size[i]= (unsigned int) ( fabs( (end[i]-origin[i])/sp[i] ) );
	}

	this->m_ReferenceSamplingGrid->SetOrigin( origin );
	this->m_ReferenceSamplingGrid->SetDirection( this->m_ReferenceImage->GetDirection() );
	this->m_ReferenceSamplingGrid->SetRegions( size );
	this->m_ReferenceSamplingGrid->SetSpacing( sp );
	this->m_ReferenceSamplingGrid->Allocate();

#ifndef NDEBUG
	typedef itk::VectorResampleImageFilter< ReferenceImageType, ReferenceImageType > Int;
	typename Int::Pointer intp = Int::New();
	intp->SetInput( this->m_ReferenceImage );
	intp->SetOutputSpacing( this->m_ReferenceSamplingGrid->GetSpacing() );
	intp->SetOutputDirection( this->m_ReferenceSamplingGrid->GetDirection() );
	intp->SetOutputOrigin( this->m_ReferenceSamplingGrid->GetOrigin() );
	intp->SetSize( this->m_ReferenceSamplingGrid->GetLargestPossibleRegion().GetSize() );
	intp->Update();

	typedef itk::ImageFileWriter< ReferenceImageType > W;
	typename W::Pointer w = W::New();
	w->SetInput( intp->GetOutput() );
	w->SetFileName( "ReferenceSamplingGridTest.nii.gz" );
	w->Update();
#endif
}

template <typename TReferenceImageType, typename TCoordRepType>
void
MahalanobisLevelSets<TReferenceImageType,TCoordRepType>
::Initialize() {
	Superclass::Initialize();

	// Initialize interpolators
	this->m_Interp->SetInputImage( this->m_ReferenceImage );
	this->m_SparseToDenseResampler->CopyImageInformation( this->m_DeformationField );

	this->m_Parameters.resize( this->m_Parameters.size() + 1 ); // Add 1 slot for the "null" region

	// Check that parameters are initialized
	if (! this->ParametersInitialized() )
		this->ComputeParameters();


}

//template <typename TReferenceImageType, typename TCoordRepType>
//typename MahalanobisLevelSets<TReferenceImageType,TCoordRepType>::MeasureType
//MahalanobisLevelSets<TReferenceImageType,TCoordRepType>
//::GetValue() {
//	return Superclass::GetValue();
//}

template <typename TReferenceImageType, typename TCoordRepType>
inline typename MahalanobisLevelSets<TReferenceImageType,TCoordRepType>::MeasureType
MahalanobisLevelSets<TReferenceImageType,TCoordRepType>
::GetEnergyAtPoint( typename MahalanobisLevelSets<TReferenceImageType,TCoordRepType>::PixelPointType & point, size_t roi ) {
	PixelType dist = this->m_Interp->Evaluate( point ) - this->m_Parameters[roi].mean;
	return dot_product(dist.GetVnlVector(), this->m_Parameters[roi].invcov.GetVnlMatrix() * dist.GetVnlVector() );
}

template <typename TReferenceImageType, typename TCoordRepType>
void MahalanobisLevelSets<TReferenceImageType,TCoordRepType>
::ComputeParameters() {
	// Update regions
	for( size_t roi = 0; roi < this->m_Parameters.size(); roi++ ) {
		ParametersType param = this->UpdateParametersOfRegion(roi);
		this->SetParameters(roi, param);

//#ifndef DNDEBUG
//		std::cout << "Region " << roi << ":" << std::endl;
//		std::cout << "\tMean = " << this->m_Parameters[roi].mean << std::endl;
//		std::cout << "\tCovariance Matrix " << std::endl << this->m_Parameters[roi].cov << std::endl;
//		std::cout << "\tCovariance Matrix^-1" << std::endl << this->m_Parameters[roi].invcov << std::endl;
//#endif
	}



}

template <typename TReferenceImageType, typename TCoordRepType>
typename MahalanobisLevelSets<TReferenceImageType,TCoordRepType>::ParametersType
MahalanobisLevelSets<TReferenceImageType,TCoordRepType>
::UpdateParametersOfRegion( const size_t idx ) {
	ParametersType newParameters;

	ROIConstPointer map = this->GetCurrentRegion( idx );

	// Resample to reference image resolution
	ResampleROIFilterPointer resampleFilter = ResampleROIFilterType::New();
	resampleFilter->SetInput( map );
	resampleFilter->SetSize( this->m_ReferenceImage->GetLargestPossibleRegion().GetSize() );
	resampleFilter->SetOutputOrigin(    this->m_ReferenceImage->GetOrigin() );
	resampleFilter->SetOutputSpacing(   this->m_ReferenceImage->GetSpacing() );
	resampleFilter->SetOutputDirection( this->m_ReferenceImage->GetDirection() );
	resampleFilter->SetDefaultPixelValue( 0.0 );
	resampleFilter->Update();

	ProbabilityMapConstPointer roipm = resampleFilter->GetOutput();

	// Apply weighted mean/covariance estimators from ITK
	typename CovarianceFilter::WeightArrayType weights;
	ReferenceSamplePointer sample = ReferenceSampleType::New();
	sample->SetImage( this->m_ReferenceImage );

	size_t sampleSize = this->m_ReferenceImage->GetLargestPossibleRegion().GetNumberOfPixels();
	weights.SetSize( sampleSize );
	weights.Fill( 0.0 );

	const typename ProbabilityMapType::PixelType* roipmb = roipm->GetBufferPointer();
	for ( size_t pidx = 0; pidx < sampleSize; pidx++) {
		weights[pidx] = *( roipmb + pidx );
	}

	CovarianceFilterPointer covFilter = CovarianceFilter::New();
	covFilter->SetInput( sample );
	covFilter->SetWeights( weights );
	covFilter->Update();

	newParameters.mean = covFilter->GetMean();
	typename CovarianceFilter::MatrixType cov = covFilter->GetCovarianceMatrix();

	for (size_t row = 0; row < Components; row ++ ) {
		for (size_t col = 0; col < Components; col ++ ) {
			newParameters.cov(row,col) = cov(row,col);
		}
	}

#ifndef DNDEBUG
	typedef itk::ImageFileWriter< ProbabilityMapType > ROIWriter;
	typename ROIWriter::Pointer w = ROIWriter::New();
	w->SetInput( roipm );
	std::stringstream ss;
	ss << "roi_transformed_lr_" << std::setfill( '0' ) << std::setw(2) << idx << ".nii.gz";
	w->SetFileName( ss.str().c_str() );
	w->Update();
#endif

	return newParameters;
}




template <typename TReferenceImageType, typename TCoordRepType>
size_t
MahalanobisLevelSets<TReferenceImageType,TCoordRepType>
::AddShapePrior( typename MahalanobisLevelSets<TReferenceImageType,TCoordRepType>::ContourDeformationType* prior,
		         typename MahalanobisLevelSets<TReferenceImageType,TCoordRepType>::ParametersType& params){
	size_t id = this->AddShapePrior( prior );
	this->SetParameters( id, params );
}

template <typename TReferenceImageType, typename TCoordRepType>
size_t
MahalanobisLevelSets<TReferenceImageType,TCoordRepType>
::AddShapePrior( typename MahalanobisLevelSets<TReferenceImageType,TCoordRepType>::ContourDeformationType* prior ){
	this->Superclass::AddShapePrior( prior );

	size_t id = this->m_Parameters.size();
	this->m_Parameters.resize( id + 1 );
	this->m_Parameters.back().initialized = false;

	return id;
}

template <typename TReferenceImageType, typename TCoordRepType>
void
MahalanobisLevelSets<TReferenceImageType,TCoordRepType>
::SetParameters( size_t roi,
		         typename MahalanobisLevelSets<TReferenceImageType,TCoordRepType>::ParametersType& params ) {

	this->m_Parameters[roi].mean = params.mean;

	CovarianceType cov = params.cov;

	if (cov.GetVnlMatrix().rows() != cov.GetVnlMatrix().cols()) {
		itkExceptionMacro(<< "Covariance matrix must be square");
	}

	if ( cov.GetVnlMatrix().rows() != Components ) {
		itkExceptionMacro(<< "Length of measurement vectors must be the same as the size of the covariance.");
	}

	PixelValueType det = 0.0;

	if( Components > 1 ) {
		// Compute diagonal and check that eigenvectors >= 0.0
		typedef typename vnl_diag_matrix<PixelValueType>::iterator DiagonalIterator;
		typedef vnl_symmetric_eigensystem<PixelValueType> Eigensystem;
		vnl_matrix< PixelValueType > vnlCov = cov.GetVnlMatrix();
		Eigensystem* e = new Eigensystem( vnlCov );

		bool modified = false;
		DiagonalIterator itD = e->D.begin();
		while ( itD!= e->D.end() ) {
			if (*itD < 0) {
				*itD = 0.;
				modified = true;
			}
			itD++;
		}

		if (modified)
			this->m_Parameters[roi].cov = e->recompose();
		else
			this->m_Parameters[roi].cov = cov;
		delete e;

		// the inverse of the covariance matrix is first computed by SVD
		vnl_matrix_inverse< PixelValueType > inv_cov( cov.GetVnlMatrix() );

		// the determinant is then costless this way
		det = inv_cov.determinant_magnitude();

		if( det < 0.) {
			itkExceptionMacro( << "| sigma | < 0" );
		}

		// FIXME Singurality Threshold for Covariance matrix: 1e-6 is an arbitrary value!!!
		const PixelValueType singularThreshold = 1.0e-10;
		if( det > singularThreshold ) {
			// allocate the memory for inverse covariance matrix
			this->m_Parameters[roi].invcov = inv_cov.inverse();
		} else {
			// TODO Perform cholesky diagonalization and select the semi-positive aproximation
			vnl_matrix< double > diag_cov( Components, Components );
			for ( size_t i = 0; i<Components; i++)
				for ( size_t j = 0; j<Components; j++)
					diag_cov[i][j] = vnlCov[i][j];
			vnl_ldl_cholesky* chol = new vnl_ldl_cholesky( diag_cov );
			vnl_vector< double > D( chol->diagonal() );
			det = dot_product( D, D );
			vnl_matrix_inverse< double > R (chol->upper_triangle());

			for ( size_t i = 0; i<Components; i++)
				for ( size_t j = 0; j<Components; j++)
					this->m_Parameters[roi].invcov(i,j) = R.inverse()[i][j];
		}
	} else {
		this->m_Parameters[roi].invcov(0,0)=1.0 / cov(0,0);
		det = fabs( this->m_Parameters[roi].cov(0,0) );
	}
	this->m_Parameters[roi].bias = Components * log( 2*vnl_math::pi ) + log( det );
	this->m_Parameters[roi].initialized = true;
}

template< typename TReferenceImageType, typename TCoordRepType >
bool
MahalanobisLevelSets<TReferenceImageType, TCoordRepType>
::ParametersInitialized() const {
	for ( size_t i = 0; i < this->m_Parameters.size(); i++ ) {
		if ( ! this->m_Parameters[i].initialized ) return false;
	}
	return true;
}

}

#endif /* MAHALANOBISLEVELSETS_HXX_ */
