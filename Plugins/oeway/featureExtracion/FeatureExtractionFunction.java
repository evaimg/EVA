package plugins.oeway.featureExtraction;

import icy.type.point.Point5D;

import java.util.ArrayList;
import java.util.HashMap;

import plugins.adufour.ezplug.EzVar;

public interface FeatureExtractionFunction{
	//initialize
	public void initialize(HashMap<String,Object> options,ArrayList<EzVar<?>> gui );

	//extract features
	public double[] process(double[] input, Point5D position);
	
	//start batch process
	public void batchBegin();
	//update progress
	public void updateProgress(int currentStep,int totalStep);
	//end batch process
	public void batchEnd();
}
