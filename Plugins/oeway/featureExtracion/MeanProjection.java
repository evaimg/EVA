package plugins.oeway.featureExtraction;

import icy.math.ArrayMath;
import icy.type.point.Point5D;

import java.util.ArrayList;
import java.util.HashMap;

import plugins.adufour.ezplug.EzLabel;
import plugins.adufour.ezplug.EzVarBoolean;

public class MeanProjection extends featureExtractionPlugin{
	@Override
	public double[] process(double[] input, Point5D position) {
		double[] output = new double[]{
				ArrayMath.mean(input)
		};

		return output;
	}
	
}
