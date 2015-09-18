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
#ifndef itkIntegrityMetricMembershipFunction_h
#define itkIntegrityMetricMembershipFunction_h

#include "itkVariableSizeMatrix.h"

#include "itkFunctionBase.h"

namespace itk
{
namespace Statistics
{
/** \class IntegrityMetricMembershipFunction
 * \brief IntegrityMetricMembershipFunction models class
 * membership using Mahalanobis distance and Euclidean distance.
 *
 * IntegrityMetricMembershipFunction is a subclass of
 * FunctionBase that models class membership (or likelihood)
 * using the Mahalanobis-weighted Euclidean distance.
 *
 * \ingroup ITKStatistics
 */

template< typename TMatrix, unsigned int MeasurementSize >
class IntegrityMetricMembershipFunction:
  public FunctionBase< TMatrix, bool >
{
public:
  /** Standard class typedefs */
  typedef IntegrityMetricMembershipFunction     Self;
  typedef FunctionBase< TMatrix, bool >         Superclass;
  typedef SmartPointer< Self >                  Pointer;
  typedef SmartPointer< const Self >            ConstPointer;

  /** Strandard macros */
  itkTypeMacro(IntegrityMetricMembershipFunction, FunctionBase);
  itkNewMacro(Self);

  /** Typedef alias for the measurement matrix */
  typedef TMatrix MeasurementMatrixType;

  /** Type of the mean vector.  */
  // HACK: Can it be a vector type that its size derives from TMatrix?
  typedef VariableLengthVector< double >  MeanVectorType;

  /** Type of the covariance matrix */
  typedef VariableSizeMatrix< double >    CovarianceMatrixType;

  /** Set threshold */
  itkSetMacro(Threshold, double);

  /** Compute mean from the input measurement matrix  */
  void ComputeMean(void);

  /** Get the mean of the Mahalanobis distance. Mean is a vector type
   * similar to the measurement type but with a real element type. */
  itkGetConstReferenceMacro(Mean, MeanVectorType);

  /** Compute the covariance matrix from the input measurement matrix */
  void ComputeCovariance(void);

  /** Get the covariance matrix. Covariance matrix is a
   * VariableSizeMatrix of doubles. */
  itkGetConstReferenceMacro(Covariance, CovarianceMatrixType);

  /**
   * Evaluate the Mahalanobis distance of a measurement using the
   * prescribed mean and covariance. Note that the Mahalanobis
   * distance is not a probability density. The square of the
   * distance is returned. */
  double Evaluate(const MeasurementMatrixType & measurement) const ITK_OVERRIDE;

protected:
  IntegrityMetricMembershipFunction();
  virtual ~IntegrityMetricMembershipFunction(void) {}
  void PrintSelf(std::ostream & os, Indent indent) const ITK_OVERRIDE;

private:
  double               m_Threshold;          // threshold value
  MeanVectorType       m_Mean;               // mean
  CovarianceMatrixType m_Covariance;         // covariance matrix

  // inverse covariance matrix. automatically calculated
  // when covariace matirx is set.
  CovarianceMatrixType m_InverseCovariance;

  /** Boolean to cache whether the covarinace is singular or nearly singular */
  bool m_CovarianceNonsingular;
};
} // end of namespace Statistics
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkIntegrityMetricMembershipFunction.hxx"
#endif

#endif
