package plugins.oeway.featureExtraction;

import icy.math.ArrayMath;
import icy.type.point.Point5D;

import java.util.ArrayList;
import java.util.HashMap;

import plugins.adufour.ezplug.EzLabel;
import plugins.adufour.ezplug.EzVar;
import plugins.adufour.ezplug.EzVarBoolean;

public class Projections extends featureExtractionPlugin {
	@Override
	public void initialize(HashMap<String,Object> options, ArrayList<Object> optionUI) {
		EzVarBoolean  b  = new EzVarBoolean("hello world", false);
		EzLabel b2 = new EzLabel("hello");
		optionUI.add(b2);
		optionUI.add( b);
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
