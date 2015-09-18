/*
 * Author: Ali Ghayoor
 * at Psychiatry Imaging Lab,
 * University of Iowa Health Care 2011
 */

#include "itkVariableSizeMatrix.h"
#include "itkMahalanobisDistanceMetric.h"
#include "itkEuclideanDistanceMetric.h"

#include "vnl/vnl_vector.h"
#include "vnl/vnl_matrix.h"
#include "vnl/algo/vnl_matrix_inverse.h"

int main( int argc, char * argv[] )
{

  // create a sample test ListSample object
  const unsigned int MeasurementVectorLength = 8;
  typedef itk::Vector< float, MeasurementVectorLength > MeasurementVectorType;
  typedef itk::Statistics::ListSample< MeasurementVectorType > SampleType;
  SampleType::Pointer sample = SampleType::New();
  sample->SetMeasurementVectorSize( MeasurementVectorLength );
  MeasurementVectorType mv;
  mv[0] = 0;
  mv[1] = 0;
  mv[2] = 3;
  mv[3] = -3;
  mv[4] = 3;
  mv[5] = -3;
  mv[6] = 3;
  mv[7] = -3;
  mv[8] = 0;
  sample->PushBack( mv );

  mv[0] = 1;
  mv[1] = -1;
  mv[2] = 1;
  mv[3] = 1;
  mv[4] = -1;
  mv[5] = -1;
  mv[6] = 0;
  mv[7] = 0;
  mv[8] = 3;
  sample->PushBack( mv );

  // compute mean and covariance
  typedef itk::Statistics::CovarianceSampleFilter< SampleType > CovarianceAlgorithmType;
  CovarianceAlgorithmType::Pointer covarianceAlgorithm = CovarianceAlgorithmType::New();
  covarianceAlgorithm->SetInput( sample );
  covarianceAlgorithm->Update();

  typename CovarianceAlgorithmType::MeasurementVectorRealType mean = covarianceAlgorithm->GetMean();
  typename CovarianceAlgorithmType::MatrixType cov = covarianceAlgorithm->GetCovarianceMatrix();



  // compute mean
  // compute covariance
  // compute MD and ED
  // create Dist
  // return isPure!
/*
  vnl_matrix<double> measurementMatrix(2,8);
  measurementMatrix.fill(1);

  // 1-compute mean
  vnl_vector<double> mean( measurementMatrix.rows() );
  for( unsigned int c = 0; c != measurementMatrix.rows(); ++c )
  {
  mean[c] = measurementMatrix.get_row(c).mean();
  }
*/





  return EXIT_SUCCESS;
}
