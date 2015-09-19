/*
 * Author: Ali Ghayoor
 * at Psychiatry Imaging Lab,
 * University of Iowa Health Care 2011
 */

#include "itkVector.h"
#include "itkListSample.h"
#include "itkCovarianceSampleFilter.h"
#include "itkMahalanobisDistanceMetric.h"
#include "itkEuclideanDistanceMetric.h"

/*
#include "itkVariableSizeMatrix.h"

#include "vnl/vnl_vector.h"
#include "vnl/vnl_matrix.h"
#include "vnl/algo/vnl_matrix_inverse.h"
*/

int main( int argc, char * argv[] )
{

  const float threshold = 2.5;

  // create a sample test ListSample object
  const unsigned int numberOfSubSamples = 9;
  const unsigned int MeasurementVectorLength = 2; // number of modality images
  typedef itk::Vector< float, MeasurementVectorLength > MeasurementVectorType;
  typedef itk::Statistics::ListSample< MeasurementVectorType > SampleType;
  SampleType::Pointer sample = SampleType::New();
  sample->SetMeasurementVectorSize( MeasurementVectorLength );
  MeasurementVectorType mv;
  mv[0] = 0;
  mv[1] = 1;
  sample->PushBack( mv );

  mv[0] = 0;
  mv[1] = -1;
  sample->PushBack( mv );

  mv[0] = 3;
  mv[1] = 1;
  sample->PushBack( mv );

  mv[0] = -3;
  mv[1] = 1;
  sample->PushBack( mv );

  mv[0] = 3;
  mv[1] = -1;
  sample->PushBack( mv );

  mv[0] = -3;
  mv[1] = -1;
  sample->PushBack( mv );

  mv[0] = -3;
  mv[1] = 0;
  sample->PushBack( mv );

  mv[0] = 3;
  mv[1] = 0;
  sample->PushBack( mv );

  mv[0] = 0;
  mv[1] = 3;
  sample->PushBack( mv );

  // compute mean and covariance
  typedef itk::Statistics::CovarianceSampleFilter< SampleType > CovarianceAlgorithmType;
  CovarianceAlgorithmType::Pointer covarianceAlgorithm = CovarianceAlgorithmType::New();
  covarianceAlgorithm->SetInput( sample );
  covarianceAlgorithm->Update();

  CovarianceAlgorithmType::MeasurementVectorRealType mean = covarianceAlgorithm->GetMean();
  CovarianceAlgorithmType::MatrixType cov = covarianceAlgorithm->GetCovarianceMatrix();

  std::cout << "Mean = " << mean << std::endl;
  std::cout << "Covariance = " << cov << std::endl;

  // Compute MD and ED
  typedef itk::Statistics::EuclideanDistanceMetric< MeasurementVectorType >  EDMetricType;
  EDMetricType::Pointer euclideanDist = EDMetricType::New();

  typedef itk::Statistics::MahalanobisDistanceMetric< MeasurementVectorType >  MDMetricType;
  MDMetricType::Pointer mahalanobisDist = MDMetricType::New();
  mahalanobisDist->SetCovariance(cov.GetVnlMatrix());

  vnl_vector<double> ed_vector( sample->Size() );
  vnl_vector<double> md_vector( sample->Size() );

  unsigned int i=0;
  for( SampleType::ConstIterator s_iter = sample->Begin(); s_iter != sample->End(); ++s_iter)
     {
     //std::cout << euclideanDist->Evaluate(mean, s_iter.GetMeasurementVector()) << std::endl;
     //std::cout << mahalanobisDist->Evaluate(mean, s_iter.GetMeasurementVector()) << std::endl;
     //std::cout << std::endl;
     ed_vector[i] = euclideanDist->Evaluate(mean, s_iter.GetMeasurementVector());
     md_vector[i] = mahalanobisDist->Evaluate(mean, s_iter.GetMeasurementVector());
     ++i;
     }

  md_vector /= md_vector.max_value();
  vnl_vector<double> dist = element_product(ed_vector, md_vector);

  std::cout << dist << std::endl;

  bool ispure = (dist.max_value() < threshold);
  std::cout << "Is pure: " << ispure << std::endl;

  return EXIT_SUCCESS;
}
