package plugins.oeway.featureExtraction;
import icy.plugin.abstract_.Plugin;
import icy.type.collection.array.Array1DUtil;
import plugins.adufour.blocks.lang.Block;
import plugins.adufour.blocks.util.VarList;
import plugins.adufour.ezplug.EzVarDoubleArrayNative;
import plugins.adufour.ezplug.EzVarInteger;
import plugins.adufour.vars.lang.Var;
import plugins.adufour.vars.util.VarListener;
public class ArraySplitor extends Plugin implements Block{
	EzVarDoubleArrayNative			varDoubleInput=new EzVarDoubleArrayNative("input",null, true);
	EzVarDoubleArrayNative			varDoubleOutputA=new EzVarDoubleArrayNative("output A",null, true);
	EzVarDoubleArrayNative			varDoubleOutputB=new EzVarDoubleArrayNative("output B",null, true);
	
	EzVarInteger varLenA = new  EzVarInteger("Length A");
	EzVarInteger varLenB = new  EzVarInteger("Length B");

	@Override
	public void declareOutput(VarList outputMap) {
		outputMap.add(varDoubleOutputA.getVariable());
		outputMap.add(varDoubleOutputB.getVariable());	
	}

	@Override
	public void run() {
		try
		{
			double[] input = varDoubleInput.getValue();
			int sizeTocopyA = input.length>=varLenA.getValue()?varLenA.getValue():input.length;
	
			if(sizeTocopyA>0)
			{
				double[] output1 = new double[sizeTocopyA];
				for(int i=0;i<sizeTocopyA;i++)
					output1[i] = input[i];
				varDoubleOutputA.setValue(output1);
			}
			else
			{
				varDoubleOutputA.setValue(new double[]{});
			}
			
			int sizeTocopyB = (input.length-sizeTocopyA)>=varLenB.getValue()?varLenB.getValue():(input.length-sizeTocopyA);
			if(sizeTocopyB>0)
			{
				double[] output2 = new double[sizeTocopyB];
				for(int i=0;i< sizeTocopyB;i++)
					output2[i] = input[sizeTocopyA+i];
				varDoubleOutputB.setValue(output2);
			}
			else
				varDoubleOutputB.setValue(new double[]{});
		}
		catch(Exception e)
		{
			
		}

	}

	@Override
	public void declareInput(VarList inputMap) {
		inputMap.add(varDoubleInput.getVariable());
		inputMap.add(varLenA.getVariable());
		inputMap.add(varLenB.getVariable());
		VarListener listener = new VarListener()
		{
			@Override
			public void valueChanged(Var source, Object oldValue, Object newValue) {
				run();
			}
	
			@Override
			public void referenceChanged(Var source, Var oldReference,
					Var newReference) {
				run();
				
			}
		};
		varDoubleInput.getVariable().addListener(listener);
		varLenA.getVariable().addListener(listener);
		varLenB.getVariable().addListener(listener);
	}

}
