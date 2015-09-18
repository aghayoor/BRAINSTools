/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef itkIntegrityMetricMembershipFunction_hxx
#define itkIntegrityMetricMembershipFunction_hxx

#include "itkIntegrityMetricMembershipFunction.h"

#include "vnl/vnl_vector.h"
#include "vnl/vnl_matrix.h"
#include "vnl/algo/vnl_matrix_inverse.h"

namespace itk
{
namespace Statistics
{
template< typename TMatrix, unsigned int MeasurementSize >
IntegrityMetricMembershipFunction< TMatrix, MeasurementSize >
::IntegrityMetricMembershipFunction()
{
  NumericTraits<MeanVectorType>::SetLength(m_Mean, MeasurementSize);
  m_Mean.Fill(0.0f);

  m_Covariance.SetSize(MeasurementSize, MeasurementSize);
  m_Covariance.SetIdentity();

  m_InverseCovariance = m_Covariance;

  m_CovarianceNonsingular = true;
}

template< typename TMatrix, unsigned int MeasurementSize >
void
IntegrityMetricMembershipFunction< TMatrix, MeasurementSize >
::ComputeMean(void)
{


  m_Mean = mean;
  this->Modified();
}

template< typename TMatrix, unsigned int MeasurementSize >
void
IntegrityMetricMembershipFunction< TMatrix, MeasurementSize >
::ComputeCovariance(void)
{


  m_Covariance = cov;

  // the inverse of the covariance matrix is first computed by SVD
  vnl_matrix_inverse< double > inv_cov( m_Covariance.GetVnlMatrix() );

  // the determinant is then costless this way
  double det = inv_cov.determinant_magnitude();

  if( det < 0.)
    {
    itkExceptionMacro( << "det( m_Covariance ) < 0" );
    }

  // 1e-6 is an arbitrary value!!!
  const double singularThreshold = 1.0e-6;
  m_CovarianceNonsingular = ( det > singularThreshold );

  if( m_CovarianceNonsingular )
    {
    // allocate the memory for m_InverseCovariance matrix
    m_InverseCovariance.GetVnlMatrix() = inv_cov.inverse();
    }
  else
    {
    // define the inverse to be diagonal with large values along the
    // diagonal. value chosen so (X-M)'inv(C)*(X-M) will usually stay
    // below NumericTraits<double>::max()
    const double aLargeDouble = std::pow(NumericTraits<double>::max(), 1.0/3.0)
      / (double) this->GetMeasurementVectorSize();
    m_InverseCovariance.SetSize(this->GetMeasurementVectorSize(), this->GetMeasurementVectorSize());
    m_InverseCovariance.SetIdentity();
    m_InverseCovariance *= aLargeDouble;
    }

  this->Modified();
}

template< typename TMatrix, unsigned int MeasurementSize >
bool
IntegrityMetricMembershipFunction< TMatrix, MeasurementSize >
::Evaluate(const MeasurementMatrixType & measurement) const
{


  return isPure;
}

template< typename TMatrix, unsigned int MeasurementSize >
void
IntegrityMetricMembershipFunction< TMatrix, MeasurementSize >
::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Mean: " << m_Mean << std::endl;
  os << indent << "Covariance: " << std::endl;
  os << m_Covariance.GetVnlMatrix();
  os << indent << "InverseCovariance: " << std::endl;
  os << indent << m_InverseCovariance.GetVnlMatrix();
  os << indent << "Covariance nonsingular: " <<
    (m_CovarianceNonsingular ? "true" : "false") << std::endl;
}

} // end namespace Statistics
} // end of namespace itk

#endif
