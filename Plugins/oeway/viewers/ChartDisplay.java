package plugins.oeway.viewers;
import icy.plugin.abstract_.Plugin;
import plugins.adufour.blocks.lang.Block;
import plugins.adufour.blocks.util.VarList;
public class ChartDisplay extends Plugin implements Block{
	EzVarChart1D chartVar = new EzVarChart1D("", null, true);
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
