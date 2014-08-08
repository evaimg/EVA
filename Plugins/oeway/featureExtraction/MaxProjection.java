package plugins.oeway.featureExtraction;

import icy.math.ArrayMath;


public class MaxProjection extends featureExtractionPlugin{
	@Override
	public double[] process(double[] input, double[] position) {
		
		return new double[]{ ArrayMath.max(input)};
	}
	
}
