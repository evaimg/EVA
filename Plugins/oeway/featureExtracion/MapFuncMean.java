package plugins.oeway.featureExtraction;

import icy.math.ArrayMath;
import icy.type.point.Point5D;

import java.util.ArrayList;
import java.util.HashMap;

import plugins.adufour.ezplug.EzVar;

class MeanProjection extends featureExtractionPlugin{
	
	@Override
	public double[] process(double[] input, Point5D position) {
		return new double[]{ ArrayMath.mean(input)};
	}


	
}
