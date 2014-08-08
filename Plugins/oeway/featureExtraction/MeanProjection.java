package plugins.oeway.featureExtraction;

import icy.math.ArrayMath;

public class MeanProjection extends featureExtractionPlugin{
	@Override
	public double[] process(double[] input, double[]  position) {
		double[] output = new double[]{
				ArrayMath.mean(input)
		};

		return output;
	}
	
}
