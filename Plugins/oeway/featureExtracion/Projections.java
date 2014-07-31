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
		EzLabel label = new EzLabel("Projections");
		EzVarBoolean  b  = new EzVarBoolean("hello world", false);
		optionUI.add(label);
		optionUI.add(b);
		options.put(FEATURE_GROUPS, new String[]{"output1","output2"});
	}
	@Override
	public double[] process(double[] input, Point5D position) {
		double[] output = new double[]{
				ArrayMath.min(input),
				ArrayMath.mean(input),
				ArrayMath.max(input),
				ArrayMath.min(input)
		};

		return output;
	}

}
