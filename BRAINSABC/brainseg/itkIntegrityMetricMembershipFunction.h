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

template< typename TSample >
class IntegrityMetricMembershipFunction:
  public FunctionBase< TSample, bool >
{
public:
  /** Standard class typedefs */
  typedef IntegrityMetricMembershipFunction     Self;
  typedef FunctionBase< TSample, bool >         Superclass;
  typedef SmartPointer< Self >                  Pointer;
  typedef SmartPointer< const Self >            ConstPointer;

  /** Strandard macros */
  itkTypeMacro(IntegrityMetricMembershipFunction, FunctionBase);
  itkNewMacro(Self);

  typedef TSample                                                     SampleType;

  /** Type of each measurement vector in sample */
  typedef typename SampleType::MeasurementVectorType                  MeasurementVectorType;

  /** Type of the length of each measurement vector */
  typedef typename SampleType::MeasurementVectorSizeType              MeasurementVectorSizeType;

  /** Type of measurement vector component value */
  typedef typename SampleType::MeasurementType                        MeasurementType;

  /** Type of a measurement vector, holding floating point values */
  typedef typename NumericTraits< MeasurementVectorType >::RealType   MeasurementVectorRealType;

  /** Type of a floating point measurement component value */
  typedef typename NumericTraits< MeasurementType >::RealType         MeasurementRealType;

  /** Type of the mean vector.  */
  typedef MeasurementVectorRealType                    MeanVectorType;

  /** Type of the covariance matrix */
  typedef VariableSizeMatrix< MeasurementRealType >    CovarianceMatrixType;

  /** Set threshold */
  itkSetMacro(Threshold, float);

  /** Get the mean of the Mahalanobis distance. Mean is a vector type
   * similar to the measurement type but with a real element type. */
  itkGetConstReferenceMacro(Mean, MeanVectorType);

  /** Get the covariance matrix. Covariance matrix is a
   * VariableSizeMatrix of doubles. */
  itkGetConstReferenceMacro(Covariance, CovarianceMatrixType);

  /**
   * Evaluate the Mahalanobis distance of a measurement using the
   * prescribed mean and covariance. Note that the Mahalanobis
   * distance is not a probability density. The square of the
   * distance is returned. */
  bool Evaluate(const SampleType & measurementSample) const ITK_OVERRIDE;

  /** Get the length of the measurement vector */
  itkGetConstMacro(MeasurementVectorSize, MeasurementVectorSizeType);

protected:
  IntegrityMetricMembershipFunction();
  virtual ~IntegrityMetricMembershipFunction(void) {}
  void PrintSelf(std::ostream & os, Indent indent) const ITK_OVERRIDE;

private:
  MeasurementVectorSizeType   m_MeasurementVectorSize;
  bool                        m_IsPure;
  float                       m_Threshold;          // threshold value
  MeanVectorType              m_Mean;               // mean
  CovarianceMatrixType        m_Covariance;         // covariance matrix
};
} // end of namespace Statistics
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkIntegrityMetricMembershipFunction.hxx"
#endif

#endif
