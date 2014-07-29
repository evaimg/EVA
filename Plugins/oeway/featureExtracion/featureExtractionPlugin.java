package plugins.oeway.featureExtraction;

import java.util.ArrayList;
import java.util.HashMap;

import plugins.adufour.blocks.lang.Block;
import plugins.adufour.blocks.util.VarList;
import plugins.adufour.ezplug.EzVar;
import plugins.adufour.ezplug.EzVarDoubleArrayNative;
import icy.plugin.abstract_.Plugin;
import icy.type.point.Point5D;

public abstract class featureExtractionPlugin extends Plugin implements FeatureExtractionFunction, Block{
	protected final String INPUT_SEQUENCE_VAR = "Input(EzVarSequence)";
	protected final String EXTRACT_AXIS = "ExtractAxis(EzVarEnum<ExtractDirection>)";
	protected final String FEATURE_GROUPS = "FeatureGroups(String[])";
	protected final String FEATURE_COUNT = "FeatureCount(int)";
	protected final String FEATURE_DATA_TYPE = "FeatureDataType(Double)";
	protected final String MAXIMUM_ERROR_COUNT = "MaximumErrorCount(int)";
	protected final String IS_RESTRICT_TO_ROI = "IsRestrictToROI(EzVarBoolean)";
	protected final String OUTPUT_SEQUENCE_VAR = "Output(EzVarSequence)";
	
	EzVarDoubleArrayNative			varDoubleInput=new EzVarDoubleArrayNative("input",null, true);
	EzVarDoubleArrayNative			varDoubleOutput=new EzVarDoubleArrayNative("output",null, true);
	
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
		varDoubleOutput.setValue(process(varDoubleInput.getValue(),new Point5D.Integer(-1,-1,-1,-1,-1)));
	}

	@Override
	public void declareInput(VarList inputMap) {
		inputMap.add(varDoubleInput.getVariable());
	}
	@Override
	public void declareOutput(VarList outputMap) {
		outputMap.add(varDoubleOutput.getVariable());
		
	}
	
}
