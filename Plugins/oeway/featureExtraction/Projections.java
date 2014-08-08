package plugins.oeway.featureExtraction;

import icy.math.ArrayMath;
import java.util.ArrayList;
import java.util.HashMap;

import plugins.adufour.ezplug.EzLabel;
import plugins.adufour.ezplug.EzVarBoolean;
import plugins.adufour.vars.lang.Var;
import plugins.adufour.vars.util.VarListener;

public class Projections extends featureExtractionPlugin {
	@Override
	public void initialize(HashMap<String,Object> options, ArrayList<Object> optionUI) {
		final EzLabel label = new EzLabel("Projections");
		EzVarBoolean  b  = new EzVarBoolean("hello world", false);
		b.getVariable().addListener(new VarListener<Boolean>(){
			@Override
			public void valueChanged(Var<Boolean> source, Boolean oldValue,
					Boolean newValue) {
				label.setText(newValue.toString());
			}
			@Override
			public void referenceChanged(Var<Boolean> source,
					Var<? extends Boolean> oldReference,
					Var<? extends Boolean> newReference) {
				label.setText(newReference.getValueAsString());
			}});
		
		
		optionUI.add(b);
		optionUI.add(label);
		options.put(FEATURE_GROUPS, new String[]{"output1","output2"});
	}
	@Override
	public double[] process(double[] input, double[] position) {
		double[] output = new double[]{
				ArrayMath.min(input),
				ArrayMath.mean(input),
				ArrayMath.max(input),
				ArrayMath.min(input)
		};

		return output;
	}

}
