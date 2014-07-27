package plugins.oeway.mapfun;

import java.util.HashMap;

import plugins.adufour.ezplug.EzGroup;

public interface MapFunction {
	//initialize
	public void initialize(EzGroup optionsGroup);
	//get and change configurations
	public void configure(HashMap<String,Object> configurations);
	//execute the function
	public boolean process(double[] input, double[] output);
	
}
