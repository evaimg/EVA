package plugins.oeway.featureExtraction;
import java.util.ArrayList;
import java.util.HashMap;

public interface FeatureExtractionFunction{
	//initialize
	public void initialize(HashMap<String,Object> options,ArrayList<Object> optionUI );

	//extract features
	public double[] process(double[] input, double[] position);
	
	//start batch process
	public void batchBegin();
	//update progress
	public void updateProgress(int currentStep,int totalStep);
	//end batch process
	public void batchEnd();
}
