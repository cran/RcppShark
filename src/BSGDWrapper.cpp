//
// BSGDWrapper.cpp
//			this is part of the RcppShark package (http://github.com/aydindemircioglu/RcppShark)
//
// Copyright (C) 2015 		Aydin Demircioglu <aydin.demircioglu/at/ini.rub.de>
//
// This file is part of the RcppShark library for GNU R.
// It is made available under the terms of the GNU General Public
// License, version 2, or at your option, any later version,
// incorporated herein by reference.
//
// This program is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE.  See the GNU General Public License for more
// details.
// 
// Please do not use this software to destroy or spy on people, environment or things.
// All negative use is prohibited.
//


#include <shark/Algorithms/Trainers/Budgeted/KernelBudgetedSGDTrainer.h> // the KernelBudgetedSGD trainer
#include <shark/Algorithms/Trainers/Budgeted/MergeBudgetMaintenanceStrategy.h> // the strategy the trainer will use
#include <shark/Algorithms/Trainers/Budgeted/AbstractBudgetMaintenanceStrategy.h> 

#include <shark/Algorithms/Trainers/Budgeted/MergeBudgetMaintenanceStrategy.h>
#include <shark/Algorithms/Trainers/Budgeted/ProjectBudgetMaintenanceStrategy.h>
#include <shark/Algorithms/Trainers/Budgeted/RemoveBudgetMaintenanceStrategy.h>

#include <shark/Data/DataDistribution.h> //includes small toy distributions
#include <shark/Models/Kernels/GaussianRbfKernel.h> //the used kernel for the SVM
#include <shark/ObjectiveFunctions/Loss/HingeLoss.h> // the loss we want to use for the SGD machine
#include <shark/ObjectiveFunctions/Loss/ZeroOneLoss.h> //used for evaluation of the classifier

#include "utils.h"
#include <Rcpp.h>


using namespace std;
using namespace Rcpp;

//' Budgeted SGD wrapper.
//'
//' @param X		matrix with training data
//' @param Y		labels for X
//' @param C		regularization constant
//' @param budget	size of budget
//' @param gamma		kernel bandwidth
//' @param epochs		number of iterations through data set 
//' @param budgetMaintenanceStrategy		strategy to use to maintain the budget size. 
//' 		choises are 'Merge', 'RemoveSmallest', 'RemoveRandom, 'Project'
//' @param verbose		verbose output?
//'
//' @export
//'
//' @note	only fo binary classification. no checks are done. always uses a bias term.
//'
// [[Rcpp::depends(BH)]]    
// [[Rcpp::export]]	
List BSGDWrapperTrain(NumericMatrix X, NumericVector Y, double C, unsigned long budget, double gamma, 
				 double epochs, std::string budgetMaintenanceStrategy, bool verbose = false) {

	try {
		AbstractBudgetMaintenanceStrategy <RealVector> *strategy = NULL;
		// check for strategy
		if (budgetMaintenanceStrategy == "RemoveSmallest") {
			strategy = new RemoveBudgetMaintenanceStrategy<RealVector> (RemoveBudgetMaintenanceStrategy<RealVector>::SMALLEST);
		}
		
		if (budgetMaintenanceStrategy == "RemoveRandom") {
			strategy = new RemoveBudgetMaintenanceStrategy<RealVector> (RemoveBudgetMaintenanceStrategy<RealVector>::RANDOM);
		}
		
		if (budgetMaintenanceStrategy == "Merge") {
			strategy = new MergeBudgetMaintenanceStrategy<RealVector>();
		}
		
		if (budgetMaintenanceStrategy == "Project") {
			strategy = new ProjectBudgetMaintenanceStrategy<RealVector>();
		}
		
		if (strategy == NULL)
			stop ("Unknown budget strategy.");
		
	
		if (verbose == true) {
			Rcout << "Parameters:\n";
			Rcout<<"\tC: \t\t" << C << "\n";
			Rcout<<"\tgamma: \t\t" << gamma << "\n";
			Rcout<<"\tbudget: \t" << budget << "\n";
			Rcout<<"\tepochs: \t" << epochs << "\n";
			Rcout<<"\tstrategy: \t" << budgetMaintenanceStrategy << "\n";
		}
		
		// probably stupid, but for now its ok
		if (verbose) Rcout << "Converting data.. " << std::endl;
		UnlabeledData<RealVector> inputs = NumericMatrixToUnlabeledData(X);
		Data<unsigned int> labels = NumericVectorToLabels (Y);
			
		ClassificationDataset trainingData ;
		trainingData.inputs() = inputs;
		trainingData.labels() = labels;
		
		// define things	
		KernelClassifier<RealVector> kc;
		GaussianRbfKernel<> kernel(gamma);


		// train
		HingeLoss hingeLoss;
		KernelBudgetedSGDTrainer<RealVector> *kernelBudgetedSGDtrainer = 
			new KernelBudgetedSGDTrainer<RealVector> (&kernel, &hingeLoss, C, true, false, budget, strategy);
		kernelBudgetedSGDtrainer -> setEpochs (epochs);
		kernelBudgetedSGDtrainer -> setMinMargin (1.0);
		kernelBudgetedSGDtrainer->train (kc, trainingData);
		
		// compute loss
		ZeroOneLoss<unsigned int> loss; // 0-1 loss
		Data<unsigned int> output = kc (trainingData.inputs()); // evaluate on training set
		double trainError = loss.eval(trainingData.labels(), output);
		if (verbose) Rcout << "Training error:\t" <<  trainError << endl;
		
		// Find the support vector
		if (verbose) Rcout << "Obtaining support vectors:\t" << endl;
		RealMatrix fAlpha = kc.decisionFunction().alpha();
		unsigned long nSV = fAlpha.size1();
		if (verbose) Rcout << "\t" << nSV << " support vectors" << endl;

		Rcpp::NumericVector alpha(nSV);
		for (unsigned long x = 0; x < nSV; x++) {
			double currentAlpha = row (fAlpha, x)[0];
			alpha[x] = currentAlpha;
		}
		
		// retrieve model data
		Data<RealVector> supportvectors = kc.decisionFunction().basis();
		RealVector offset = kc.decisionFunction().offset();

		// free memory
		delete kernelBudgetedSGDtrainer;		
		delete strategy;
		
		Rcpp::List rl = R_NilValue;
		rl = Rcpp::List::create(Rcpp::Named("error") = trainError,
        		Rcpp::Named("offset") = RealVectorToNumericVector (offset),
        		Rcpp::Named("nSV") = nSV,
				Rcpp::Named("SV") = DataRealVectorToNumericMatrix (supportvectors),
				Rcpp::Named("alpha") = alpha);
		return rl;
    } catch(...) {
		// free memory
		stop ("Unknown exception occured. Check also your budget strategy.");
	}
	
	// this should never happen, but CMD check doesnt like it like this
	Rcpp::List rl = R_NilValue;
	return rl;
}



//' Budgeted SGD wrapper.
//'
//' @param X		matrix with input data
//' @param alpha		alpha for support vectors
//' @param SV		support vectors of model
//' @param offset		offset/bias terms
//' @param verbose		verbose output?
//'
//' @note	Currently works only for binary classification. No checks are done.
//'
//' @export
//'
// [[Rcpp::depends(BH)]]    
// [[Rcpp::export]]	
List BSGDWrapperPredict(NumericMatrix X, NumericVector alpha, NumericMatrix SV, NumericVector offset, double gamma, bool verbose = false) {

	try {
		if (verbose == true) {
			Rcout << "Parameters:\n";
			Rcout<<"\tgamma: \t\t" << gamma << "\n";
		}
		
		// convert data
		if (verbose) Rcout << "Converting data.. " << std::endl;
		Data<RealVector> testData = NumericMatrixToDataRealVector (X);
		Data<RealVector> supportVectors= NumericMatrixToDataRealVector (SV);
		RealVector alphas = NumericVectorToRealVector (alpha);
		if (verbose) Rcout << "Alpha size.. " << alpha.size() << std::endl;
		RealVector offsets = NumericVectorToRealVector (offset);
		if (verbose) Rcout << "Offsets size.. " << offsets.size() << std::endl;
		
		// initialize kernel classifier
		KernelExpansion<RealVector> sharkModel;
		GaussianRbfKernel<> kernel (gamma);
		sharkModel.setStructure (&kernel, supportVectors, true, 1);
		
		// note that this works for binary problems only.
		
		// now copy over all alphas into the parameter vector
		RealVector parameters (alphas.size() + 1);
		for (size_t r = 0; r < alphas.size(); r++) {
			parameters[r] = alphas (r);
		}
		
		for (size_t c = 0; c < 1; c++) {
			parameters[alphas.size() + c] = offsets[c];
		}
		
		
		// have model its parameters.
		sharkModel.setParameterVector (parameters);
		
		
		// do prediction
		KernelClassifier<RealVector> kc (sharkModel);
		Data<unsigned int> label = kc (testData);
		NumericVector prediction = LabelsToNumericVector (label);

		Rcpp::List rl = R_NilValue;
		rl = Rcpp::List::create(Rcpp::Named("prediction") = prediction);
		return rl;
	} catch(...) {
		stop ("Unknown exception occured.");
	}
	
	// this should never happen, but CMD check doesnt like it like this
	Rcpp::List rl = R_NilValue;
	return rl;
}
				 
				 
