package plugins.oeway.featureExtraction;

import icy.math.ArrayMath;
import icy.type.point.Point5D;


class MaxProjection extends featureExtractionPlugin{
	@Override
	public double[] process(double[] input, Point5D position) {
		
		return new double[]{ ArrayMath.max(input)};
	}
	
}
