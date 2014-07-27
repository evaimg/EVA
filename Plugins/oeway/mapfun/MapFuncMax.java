package plugins.oeway.mapfun;

import icy.math.ArrayMath;
import icy.plugin.abstract_.Plugin;

import java.util.HashMap;

import plugins.adufour.ezplug.EzGroup;


class MapFuncMax extends Plugin implements MapFunction{

	@Override
	public void configure(HashMap<String, Object> configurations) {
		configurations.put("OutputChannels", new String[]{"max"});
		configurations.put("OutputLength", 1);
	}

	@Override
	public boolean process(double[] input, double[] output) {
		output[0] = ArrayMath.max(input);
		return true;
	}
	

	@Override
	public void initialize(EzGroup optionsGroup) {

	}
	
}
