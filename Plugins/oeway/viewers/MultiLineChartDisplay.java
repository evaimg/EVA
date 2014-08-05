package plugins.oeway.viewers;
import icy.plugin.abstract_.Plugin;
import plugins.adufour.blocks.lang.Block;
import plugins.adufour.blocks.util.VarList;
public class MultiLineChartDisplay extends Plugin implements Block{
	EzVarMultiChart1D chartVar = new EzVarMultiChart1D("");
	@Override
	public void declareInput(VarList inputMap) {
		inputMap.add(chartVar.getVariable());
	}

	@Override
	public void declareOutput(VarList outputMap) {
		
	}

	@Override
	public void run() {

	}

}
