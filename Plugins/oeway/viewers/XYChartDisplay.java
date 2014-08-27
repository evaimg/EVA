package plugins.oeway.viewers;
import icy.plugin.abstract_.Plugin;
import plugins.adufour.blocks.lang.Block;
import plugins.adufour.blocks.util.VarList;
import plugins.adufour.ezplug.EzVarDoubleArrayNative;
import plugins.adufour.vars.lang.Var;
import plugins.adufour.vars.util.VarListener;
public class XYChartDisplay extends Plugin implements Block{
	EzVarXYChart1D chartVar = new EzVarXYChart1D("");
	EzVarDoubleArrayNative xVar = new EzVarDoubleArrayNative("X",null,true);
	EzVarDoubleArrayNative yVar = new EzVarDoubleArrayNative("Y",null,true);
	
	@Override
	public void declareInput(VarList inputMap) {
		inputMap.add(xVar.getVariable());
		inputMap.add(yVar.getVariable());
		inputMap.add(chartVar.getVariable());
		VarListener listener = new VarListener<double[]>(){

			@Override
			public void valueChanged(Var<double[]> source, double[] oldValue,
					double[] newValue) {
				run();
			}

			@Override
			public void referenceChanged(Var<double[]> source,
					Var<? extends double[]> oldReference,
					Var<? extends double[]> newReference) {
				run();
			}
			
		};
		xVar.getVariable().addListener(listener);
		yVar.getVariable().addListener(listener);
		
	}
	
	@Override
	public void declareOutput(VarList outputMap) {
		
	}

	@Override
	public void run() {
		
		try
		{
			int len = Math.min(xVar.getValue().length, yVar.getValue().length);
			double[] x = xVar.getValue();
			double[] y = yVar.getValue();
			double[][] xy = new double[2][len];
			for(int i=0;i<len;i++)
			{
				xy[0][i] = x[i];
				xy[1][i] = y[i];
			}
			chartVar.setValue(xy);
		}
		catch(Exception e)
		{
			
		}
	}

}
