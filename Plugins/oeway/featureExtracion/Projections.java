package plugins.oeway.featureExtraction;

import icy.math.ArrayMath;
import icy.type.point.Point5D;

import java.util.ArrayList;
import java.util.HashMap;

import plugins.adufour.ezplug.EzVar;
import plugins.adufour.ezplug.EzVarBoolean;

public class Projections extends featureExtractionPlugin {
	@Override
	public void initialize(HashMap<String,Object> options,ArrayList<EzVar<?>> gui) {
		EzVarBoolean  b  = new EzVarBoolean("hello world", false);
		gui.add( b);
	}
	@Override
	public double[] process(double[] input, Point5D position) {
		double[] output = new double[]{
				ArrayMath.max(input),
				ArrayMath.mean(input),
				ArrayMath.min(input)
				};

		return output;
	}

}
