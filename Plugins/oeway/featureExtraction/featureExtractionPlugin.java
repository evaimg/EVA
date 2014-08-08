package plugins.oeway.featureExtraction;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedHashMap;

import plugins.adufour.blocks.lang.Block;
import plugins.adufour.blocks.util.VarList;
import plugins.adufour.ezplug.EzVar;
import plugins.adufour.ezplug.EzVarDoubleArrayNative;
import plugins.adufour.vars.lang.Var;
import plugins.adufour.vars.util.VarListener;
import icy.gui.dialog.MessageDialog;
import icy.plugin.PluginDescriptor;
import icy.plugin.PluginLauncher;
import icy.plugin.abstract_.PluginActionable;
import icy.util.StringUtil;


public abstract class featureExtractionPlugin extends PluginActionable implements FeatureExtractionFunction, Block{
	protected final String INPUT_SEQUENCE_VAR = "Input(EzVarSequence)";
	protected final String EXTRACT_AXIS = "ExtractAxis(EzVarEnum<ExtractDirection>)";
	protected final String FEATURE_GROUPS = "FeatureGroups(String[])";
	protected final String FEATURE_COUNT = "FeatureCount(int)";
	protected final String FEATURE_DATA_TYPE = "FeatureDataType(Double)";
	protected final String MAXIMUM_ERROR_COUNT = "MaximumErrorCount(int)";
	protected final String IS_RESTRICT_TO_ROI = "IsRestrictToROI(EzVarBoolean)";
	protected final String OUTPUT_SEQUENCE_VAR = "Output(EzVarSequence)";
	public boolean standaloneMode = true;
	
	ArrayList<Object> optionUIList = new ArrayList<Object>();
	
	EzVarDoubleArrayNative			varDoubleInput=new EzVarDoubleArrayNative("input",null, true);
	EzVarDoubleArrayNative			varDoubleOutput=new EzVarDoubleArrayNative("output",null, true);
	
	EzVarDoubleArrayNative			varDoublePos=new EzVarDoubleArrayNative("Pos",null, true);
	
    LinkedHashMap<String,Object> optionDict = new LinkedHashMap<String,Object>();
    
    private final double[] currentPos = new double[5];
	@Override
	public void initialize(HashMap<String,Object> options,ArrayList<Object> optionUI) {

	}
	
	//start batch process
	@Override
	public void batchBegin(){
		
	};
	//update progress
	@Override
	public void updateProgress(int currentStep,int totalStep){
		
	};
	//end batch process
	@Override
	public void batchEnd(){
		
	};
    
	@Override
	public void run() {
		
		if(standaloneMode)
		{
			try
			{
				FeatureExtractionEngine engine = (FeatureExtractionEngine) PluginLauncher.startSafe(FeatureExtractionEngine.class.getName());
				String name = this.getClass().getSimpleName();
				PluginDescriptor descriptor = this.getDescriptor();
				String funcName = descriptor.getName().equalsIgnoreCase(name) ? StringUtil.getFlattened(name) : descriptor.getName();
				engine.setExtractionFunction(funcName);
			}
			catch(Exception e)
			{
				e.printStackTrace();
				MessageDialog.showDialog("Extraction Function is not available",
						MessageDialog.ERROR_MESSAGE);
			}
		}
		else
		{
			try
			{

				try
				{
					double[] val = varDoublePos.getVariable().getValue();
					currentPos[0]=val[0];
					currentPos[1]=val[1];
					currentPos[2]=val[2];
					currentPos[3]=val[3];
					currentPos[4]=val[4];
				}
				catch(Exception e)
				{
					currentPos[0]=-1;
					currentPos[1]=-1;
					currentPos[2]=-1;
					currentPos[3]=-1;
					currentPos[4]=-1;
				}

				varDoubleOutput.setValue(process(varDoubleInput.getValue(),currentPos));
			}
			catch(Exception e)
			{
				e.printStackTrace();
			}
		}
	}

	@Override
	public void declareInput(VarList inputMap) {
		standaloneMode = false;
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
		
		initialize(optionDict,optionUIList);
		
		for(Object o:optionUIList){
			if(o instanceof EzVar<?>)
			{
				EzVar<?> v= (EzVar<?>)o;
				if(!inputMap.contains( v.getVariable())){
					inputMap.add(v.getVariable());	
					v.getVariable().addListener(listener);
				}
			}
			if(o instanceof Var<?>)
			{
				Var<?> v= (Var<?>)o;
				if(inputMap!=null)
					if(!inputMap.contains(v))
					{
						inputMap.add(v);
						v.addListener(listener);
					}
			}			
		}

		inputMap.add(varDoubleInput.getVariable());
		inputMap.add(varDoublePos.getVariable());
		
		varDoubleInput.getVariable().addListener(listener);
	}
	@Override
	public void declareOutput(VarList outputMap) {
		outputMap.add(varDoubleOutput.getVariable());
		
	}
	
}
