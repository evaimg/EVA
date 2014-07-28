package plugins.oeway.featureExtraction;

public enum ConfigType {
	 INPUT_SEQUENCE ("InputSequence");
	 final String EXTRACT_AXIS = "ExtractAxis";
	 final String FEATURE_GROUPS = "FeatureGroups";
	 final String FEATURE_GROUPS_NAMES = "FeatureGroupsNames";
	 final String FEATURE_COUNT = "FeatureCount";
	 final String FEATURE_DATA_TYPE = "FeatureDataType";
	 final String MAXIMUM_ERROR_COUNT = "MaximumErrorCount";
	 final String IS_RESTRICT_TO_ROI = "IsRestrictToROI";
	 final String OUTPUT_SEQUENCE = "OutputSequence";
	 private ConfigType(String name){
		 
	 }
}
