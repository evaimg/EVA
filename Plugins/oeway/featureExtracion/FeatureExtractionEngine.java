package plugins.oeway.featureExtraction;

import icy.gui.dialog.MessageDialog;
import icy.image.IcyBufferedImage;
import icy.plugin.PluginDescriptor;
import icy.plugin.PluginLoader;
import icy.plugin.abstract_.Plugin;
import icy.roi.ROI;
import icy.sequence.Sequence;
import icy.sequence.SequenceUtil;
import icy.type.DataType;
import icy.type.collection.array.Array1DUtil;
import icy.type.point.Point5D;
import icy.util.ClassUtil;
import icy.util.OMEUtil;
import icy.util.StringUtil;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

import plugins.adufour.blocks.lang.Block;
import plugins.adufour.blocks.util.VarList;
import plugins.adufour.ezplug.EzGroup;
import plugins.adufour.ezplug.EzPlug;
import plugins.adufour.ezplug.EzStoppable;
import plugins.adufour.ezplug.EzVar;
import plugins.adufour.ezplug.EzVarBoolean;
import plugins.adufour.ezplug.EzVarEnum;
import plugins.adufour.ezplug.EzVarListener;
import plugins.adufour.ezplug.EzVarSequence;
import plugins.adufour.ezplug.EzVarText;
import plugins.adufour.vars.lang.Var;
import plugins.adufour.vars.util.VarListener;


public class FeatureExtractionEngine extends EzPlug implements Block, EzStoppable
{

	protected final String INPUT_SEQUENCE_VAR = "Input(EzVarSequence)";
	protected final String EXTRACT_AXIS = "ExtractAxis(EzVarEnum<ExtractDirection>)";
	protected final String FEATURE_GROUPS = "FeatureGroups(String[])";
	protected final String FEATURE_COUNT = "FeatureCount(int)";
	protected final String FEATURE_DATA_TYPE = "FeatureDataType(Double)";
	protected final String MAXIMUM_ERROR_COUNT = "MaximumErrorCount(int)";
	protected final String IS_RESTRICT_TO_ROI = "IsRestrictToROI(EzVarBoolean)";
	protected final String OUTPUT_SEQUENCE_VAR = "Output(EzVarSequence)";
	
    public enum ExtractDirection
    {
        X,Y,Z,T//,C
    }
    
    
    private final EzVarSequence                  input          = new EzVarSequence("Input Sequence");
    
    private final EzVarEnum<ExtractDirection> extractDir  = new EzVarEnum<FeatureExtractionEngine.ExtractDirection>("Extract along", ExtractDirection.values(), ExtractDirection.Z);
    
    private final EzVarBoolean                   restrictToROI  = new EzVarBoolean("Restrict to ROI", false);
    
    private final EzVarSequence                    output         = new EzVarSequence("Output Sequence");
    
    private EzGroup featureFuncOptions = new EzGroup("Options");
    
    
    //private VarList  featureFuncOptionsVarList = new VarList();
    private VarList inputMap_ = null;
    
    LinkedHashMap<String,Object> optionDict = new LinkedHashMap<String,Object>();
    ArrayList<EzVar<?>> guiList = new ArrayList<EzVar<?>> ();
    
    private HashMap<String,Class<? extends FeatureExtractionFunction>> pluginList ;
    
    final EzVarText featureFuncVar= new EzVarText("Extraction Function", new String[]{}, 0, false);
    
    public FeatureExtractionFunction selectedExtractionFunc;
	boolean						stopFlag = false;
	int maxErrorCount = -1;
	int featureCount = -1;
    DataType outputDataType = DataType.DOUBLE;
    String[] groupNames = new String[]{""};
    
    String lastfeatureFuncVar = "";
    public void createFeatureFunc() throws InstantiationException, IllegalAccessException{
    	
    	if(lastfeatureFuncVar.equals(featureFuncVar.getValue()))
    		return;
    	
		selectedExtractionFunc = pluginList.get(featureFuncVar.getValue()).newInstance();
		featureFuncOptions.components.clear();

		for(EzVar<?> v:guiList){
			if(inputMap_!=null)
				if(inputMap_.contains(v.getVariable()))
					inputMap_.remove(v.getVariable());
		}
			
		optionDict.clear();
		guiList.clear();
		optionDict = createConfigurations();
		try
		{
			selectedExtractionFunc.initialize(optionDict,guiList);
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}
		//featureFuncOptions.addEzComponent(featureFuncOptions);
		updateFromConfigurations();
		
		for(EzVar<?> v:guiList){
			if(inputMap_!=null)
				if(!inputMap_.contains( v.getVariable()))
					inputMap_.add(( v).getVariable());	
			//if(!mainGroup.components.contains(v))
			featureFuncOptions.addEzComponent( v);
		}
		lastfeatureFuncVar = featureFuncVar.getValue();
		
		featureFuncOptions.setVisible(featureFuncOptions.components.size()>0);
			
    }

    public void buildFeatureFuncList()
    {
    	pluginList = getPluginList();
    	String tmp[] = new String[pluginList.size()];
    	int i=0;
    	for (Iterator<String> iter = pluginList.keySet().iterator(); iter.hasNext();) {
    		tmp[i++]=(String) iter.next();
    	}
    	featureFuncVar.setDefaultValues(tmp, 0, false);
    	//featureFuncVar = new EzVarText("Extraction Function", tmp, 0, false);
        featureFuncVar.addVarChangeListener(new EzVarListener<String>(){
			@Override
			public void variableChanged(EzVar<String> source, String newValue) {
				try {
					createFeatureFunc();	
				} catch (Exception e1) {
					e1.printStackTrace();
					return ;
				}
			}
        });
    	
        featureFuncVar.getVariable().addListener(new VarListener<String>(){

			@Override
			public void valueChanged(Var<String> source, String oldValue,
					String newValue) {
				try {
					createFeatureFunc();
			    	
				} catch (Exception e1) {
					e1.printStackTrace();
					return ;
				}
			}

			@Override
			public void referenceChanged(Var<String> source,
					Var<? extends String> oldReference,
					Var<? extends String> newReference) {
				try {
					createFeatureFunc();
			    	
				} catch (Exception e1) {
					e1.printStackTrace();
					return ;
				}
			}
        	
        });
        if(inputMap_!= null){
        	
        }
    }
	
    
    @Override
    protected void initialize()
    {
    	
    	addEzComponent(input);
    	addEzComponent(extractDir);
    	addEzComponent(featureFuncVar); 
    	addEzComponent(featureFuncOptions);
        restrictToROI.setToolTipText("Check this option to extract only the intensity data contained within the sequence ROI");
        addEzComponent(restrictToROI);
        
    	buildFeatureFuncList();

    }
    
    @Override
    protected void execute()
    {
    	stopFlag = false;

        switch (extractDir.getValue())
        {
	        case X:
	            output.setValue(xExtraction(input.getValue(true), true, restrictToROI.getValue()));
	        break;
	        case Y:
	            output.setValue(yExtraction(input.getValue(true), true, restrictToROI.getValue()));
	        break;
            case T:
                output.setValue(tExtraction(input.getValue(true), true, restrictToROI.getValue()));
            break;
            case Z:
                output.setValue(zExtraction(input.getValue(true), true, restrictToROI.getValue()));
            break;
            default:
                throw new UnsupportedOperationException("Extraction along " + extractDir.getValue() + " not supported");
        }

        if (getUI() != null)
        {
        	addSequence(output.getValue());
        	//Viewer viewer = output.getValue().getViewers().get(0);
        }
        
    }
    
    @Override
    public void clean()
    {
        
    }
    
    public  HashMap<String,Class<? extends FeatureExtractionFunction>> getPluginList()
    {
    	 HashMap<String,Class<? extends FeatureExtractionFunction>> pl =  new HashMap<String,Class<? extends FeatureExtractionFunction>>();
    	 ArrayList<PluginDescriptor> plugins = PluginLoader.getPlugins(FeatureExtractionFunction.class, true, false, false);
         
         if (plugins.size() == 0)
         {
             return pl;
         }
         
         for (PluginDescriptor descriptor : plugins)
         {
             Class<? extends Plugin> clazz = descriptor.getPluginClass();
             try
             {
                 final Class<? extends FeatureExtractionFunction> funcClass = clazz.asSubclass(FeatureExtractionFunction.class);

                 if (ClassUtil.isAbstract(funcClass) || ClassUtil.isPrivate(funcClass)) continue;
                 String name = funcClass.getSimpleName();
                 name = descriptor.getName().equalsIgnoreCase(name) ? StringUtil.getFlattened(name) : descriptor.getName();
                 pl.put(name,funcClass);

             }
             catch (ClassCastException e1)
             {
             }
         }
         return pl;
         
    }
    
	public  LinkedHashMap<String,Object>  createConfigurations(){
		LinkedHashMap<String,Object> configurations = new LinkedHashMap<String,Object>();
		
		maxErrorCount = -1;
		featureCount = -1;
	    outputDataType = DataType.DOUBLE;
	    groupNames = new String[]{""};
	    
	    configurations.put(INPUT_SEQUENCE_VAR, input);
	    configurations.put(EXTRACT_AXIS, extractDir);
		configurations.put(FEATURE_DATA_TYPE,outputDataType);
		configurations.put(MAXIMUM_ERROR_COUNT, maxErrorCount );//new EzVarInteger("Maximum Error Count"));
		configurations.put(FEATURE_GROUPS, groupNames);
		configurations.put(FEATURE_COUNT,featureCount );
		configurations.put(IS_RESTRICT_TO_ROI, restrictToROI);
		configurations.put(OUTPUT_SEQUENCE_VAR, output);
		return configurations;
	}
	public void  updateFromConfigurations(){

	    //configurations.put(INPUT_SEQUENCE_VAR, input);
	    //configurations.put(EXTRACT_AXIS, extractDir);
	    outputDataType = (DataType) optionDict.get(FEATURE_DATA_TYPE);
	    maxErrorCount = (int) optionDict.get(MAXIMUM_ERROR_COUNT);//, maxErrorCount );//new EzVarInteger("Maximum Error Count"));
		groupNames = (String[]) optionDict.get(FEATURE_GROUPS);//, groupNames);
		featureCount = (int) optionDict.get(FEATURE_COUNT);//,featureCount );
		//configurations.put(IS_RESTRICT_TO_ROI, restrictToROI);
		//configurations.put(OUTPUT_SEQUENCE_VAR, output);

	}
    
    /**
     * Performs a Z extraction of the input sequence using the specified algorithm. If the sequence
     * is already 2D, then a copy of the sequence is returned
     * 
     * @param sequence
     *            the sequence to extract
     * @param multiThread
     *            true if the process should be multi-threaded
     * @return the Extracted sequence
     */
    public Sequence zExtraction(final Sequence in, boolean multiThread)
    {
        return zExtraction(in, multiThread, false);
    }
    
    public int testOutputLength(Sequence in, FeatureExtractionFunction featureFunc)
    {
    	double[] input;
        switch (extractDir.getValue())
        {
	        case X:
	        	input = new double[in.getSizeX()];
	        break;
	        case Y:
	        	input = new double[in.getSizeY()];
	        break;
            case T:
            	input = new double[in.getSizeT()];
            break;
            case Z:
            	input = new double[in.getSizeZ()];
            break;
            default:
                throw new UnsupportedOperationException("Extraction along " + extractDir.getValue() + " not supported");
        }
        
        try{
        	//test
			double[] output = featureFunc.process(input,new Point5D.Integer(-1,-1,-1,-1,-1));
			return output.length;
		}
		catch(Exception e){
			System.err.println("Error when excuting process().");
			e.printStackTrace();
			return 1;
		}
    }
    
    /**
     * Performs a Z extraction of the input sequence using the specified algorithm. If the sequence
     * is already 2D, then a copy of the sequence is returned
     * 
     * @param sequence
     *            the sequence to extract
     * @param multiThread
     *            true if the process should be multi-threaded
     * @param restrictToROI
     *            <code>true</code> maps only data located within the sequence ROI,
     *            <code>false</code> maps the entire data set
     * @return the Extracted sequence
     */
    @SuppressWarnings("unchecked")
	public Sequence zExtraction(final Sequence in, boolean multiThread, final boolean restrictToROI)
    {
        final Sequence out = new Sequence(OMEUtil.createOMEMetadata(in.getMetadata()));
        out.setName("Extraction of " + in.getName() +" along Z");
        final FeatureExtractionFunction featureFunc = selectedExtractionFunc;
        if(selectedExtractionFunc==null)
        {
			MessageDialog.showDialog("Extraction Function is not available",
					MessageDialog.ERROR_MESSAGE);
			return null;
        }    	
    	
        try{
			featureFunc.batchBegin();
		}
		catch(Exception e){
			System.err.println("Error when excuting batchBegin().");
			e.printStackTrace();
			return null;
		} 
        updateFromConfigurations();
        
        if(featureCount<1) featureCount = testOutputLength(in,featureFunc)/groupNames.length;
        
        final List<ROI> rois = in.getROIs();
        int cpu = Runtime.getRuntime().availableProcessors();

        final int depth = in.getSizeZ();
        final int width = in.getSizeX();
        final int height = in.getSizeY();
        final int length = in.getSizeT();
        final int channels = in.getSizeC();
        
        final DataType dataType = in.getDataType_();
        

		final int error_exit_count = maxErrorCount;//((EzVarInteger) optionDict.get(MAXIMUM_ERROR_COUNT)).getValue();

        final int width_out = in.getSizeX();
        final int height_out = in.getSizeY();
        final int t_length_out = in.getSizeT();
        final int channel_scale_out = groupNames.length;// ((EzVarInteger)optionDict.get(FEATURE_GROUPS)).getValue();
        final int channels_out = in.getSizeC()*channel_scale_out; 
        final int depth_out = featureCount;//((EzVarInteger)optionDict.get(FEATURE_COUNT)).getValue();
        final DataType dataType_out = outputDataType;//(DataType) optionDict.get(FEATURE_DATA_TYPE);

        // this plug-in processes each channel of each stack in a separate thread
        ExecutorService service = multiThread ? Executors.newFixedThreadPool(cpu) : Executors.newSingleThreadExecutor();
        ArrayList<Future<?>> futures = new ArrayList<Future<?>>(channels*length *height *width);

        //Allocate output sequence 
    	for(int t=0; t<t_length_out; ++t) {
			for(int z=0; z<depth_out; ++z) {
				out.setImage(t, z, new IcyBufferedImage(width_out, height_out, channels_out, dataType_out));
			}
		}
        //out.getColorModel().setColorMap(c, in.getColorModel().getColorMap(c), true);
    	
        for (int t = 0; t < length; t++)
        {
        	if(stopFlag)
        		break;
            final int time = t;
            
            final Object out_Z_C_XY = out.getDataXYCZ(time); //[Z][C][XY]
            
            for (int c = 0; c < channels; c++)
            {
            	if(stopFlag)
            		break;
                final int channel = c;
                //set channel name
                for (int co = 0; co < channel_scale_out; co++)
            	{
                	out.setChannelName(channel_scale_out*channel+co, in.getChannelName(c)+ "/"+groupNames[co]);//channel_names[co]);
            	}

                final Object in_Z_XY = in.getDataXYZ(time, channel);
                int off = 0;
                for (int y = 0; y < height; y++)
                {
                	if(stopFlag)
                		break;
                    for (int x = 0; x < width; x++, off++)
                    {
                    	if(stopFlag)
                    		break;
                    	final int offf=off;
                    	final int yy=y;
                    	final int xx=x;
                        futures.add(service.submit(new Runnable()
                        {
                            @Override
                            public void run()
                            {
                            	int errorCount = 0;
                                double[] lineToInput = new double[depth];
                                
                                if (restrictToROI && rois.size() > 0)
                                {
                                    int nbValues = 0;
                                    
                                    for (int z = 0; z < depth; z++)
                                    {
                                        for (ROI roi : rois)
                                        {
                                            if (roi.contains(xx, yy, z, time, channel))
                                            {
                                                lineToInput[nbValues++] = Array1DUtil.getValue(((Object[]) in_Z_XY)[z], offf, dataType);
                                                break;
                                            }
                                        }
                                    }
                                    
                                    if (nbValues == 0) return;
                                    
                                    if (nbValues < lineToInput.length) lineToInput = Arrays.copyOf(lineToInput, nbValues);
                                }
                                else
                                {
                                    for (int z = 0; z < depth; z++)
                                        lineToInput[z] = Array1DUtil.getValue(((Object[]) in_Z_XY)[z], offf, dataType);
                                }
                                
                                double[] lineToOutput = null; //[C][Z]
                                
                                //int outputLength = channel_scale_out*depth_out;
                                try
                                {
                                	lineToOutput = featureFunc.process(lineToInput, new Point5D.Integer(xx,yy,-1,time,channel));
		                               if(lineToOutput!=null)
		                               {
		                            	   //if(lineToOutput.length>=outputLength)
		                            	   {
				                                if (restrictToROI && rois.size() > 0)
				                                {
				                                    int nbValues = 0;
				                                    for (int co = 0; co < channel_scale_out; co++)
			                                    	{
					                                    for (int z = 0; z < depth_out; z++)
					                                    {
					                                        for (ROI roi : rois)
					                                        {
					                                            if (roi.contains(xx, yy, z, time, channel))
					                                            {
					                                            	
					                                            	Array1DUtil.setValue(((Object[][]) out_Z_C_XY)[z][channel_scale_out*channel+co], offf, dataType_out,lineToOutput[nbValues++]);
					                                            	
					                                                break;
					                                            }
					                                        }
					                                    }
			                                    	}
				                                    
				                                    if (nbValues == 0) return;
				                                    
				                                    if (nbValues < lineToOutput.length) lineToOutput = Arrays.copyOf(lineToOutput, nbValues);
				                                }
				                                else
				                                {
				                                	int nbValues = 0;
			                                    	for (int co = 0; co < channel_scale_out; co++)
			                                    	{
			                                    		for (int z = 0; z < depth_out; z++)
			                                    			Array1DUtil.setValue(((Object[][]) out_Z_C_XY)[z][channel_scale_out*channel+co], offf, dataType_out,lineToOutput[nbValues++]);
			                                    	}
				                                }
		                            	   }
		                               }
                                }
                                catch(Exception e)
                                {
                                	
                                	System.err.println("Error when excuting Extraction Fucntion, t:"+time+" c:"+channel+" x:"+xx+" y:"+yy);
                                	
                            		e.printStackTrace();
                            		
                            		
                            		if(errorCount++>=error_exit_count){
                            			MessageDialog.showDialog("Stoped because of error! t:"+time+" c:"+channel+" x:"+xx+" y:"+yy,
                        						MessageDialog.ERROR_MESSAGE);
                            			stopFlag = error_exit_count<0?stopFlag:true; //if error_exit_count == -1 then do not stop
                            			return;
                            		}
                                		

                                }   
                               // Array1DUtil.doubleArrayToSafeArray(lineToOutput, out_Z_XY, dataType.isSigned());
                            }
                        }));
                    } // x
                } // y
            }
        }
        
        try
        {
            for (Future<?> future : futures)
                future.get();
        }
        catch (InterruptedException e)
        {
            e.printStackTrace();
        }
        catch (ExecutionException e)
        {
            e.printStackTrace();
        }
        
        service.shutdown();
        
        out.updateChannelsBounds(true);
        
    	try{
			featureFunc.batchEnd();
		}
		catch(Exception e){
			System.err.println("Error when excuting batchEnd().");
			e.printStackTrace();
			return null;
		}
    	
        return out;
    }
    /**
     * Performs a X extraction of the input sequence using the specified algorithm. If the sequence
     * is already 2D, then a copy of the sequence is returned
     * 
     * @param sequence
     *            the sequence to extract
     * @param multiThread
     *            true if the process should be multi-threaded
     * @param restrictToROI
     *            <code>true</code> maps only data located within the sequence ROI,
     *            <code>false</code> maps the entire data set
     * @return the Extracted sequence
     */
    @SuppressWarnings("unchecked")
	public Sequence xExtraction(final Sequence in, boolean multiThread, final boolean restrictToROI)
    {
        final Sequence out = new Sequence(OMEUtil.createOMEMetadata(in.getMetadata()));
        out.setName("Extraction of " + in.getName() +" along X");
        final FeatureExtractionFunction featureFunc = selectedExtractionFunc;
        if(selectedExtractionFunc==null)
        {
			MessageDialog.showDialog("Extraction Function is not available",
					MessageDialog.ERROR_MESSAGE);
			return null;
        }
        
    	try{
			featureFunc.batchBegin();
		}
		catch(Exception e){
			System.err.println("Error when excuting batchBegin().");
			e.printStackTrace();
			return null;
		}
    	updateFromConfigurations();
    	if(featureCount<1) featureCount = testOutputLength(in,featureFunc)/groupNames.length;
    	
        final List<ROI> rois = in.getROIs();
        int cpu = Runtime.getRuntime().availableProcessors();

        final int depth = in.getSizeZ();
        final int width = in.getSizeX();
        final int height = in.getSizeY();
        final int length = in.getSizeT();
        final int channels = in.getSizeC();
        
        final DataType dataType = in.getDataType_();
        
 
		final int error_exit_count = maxErrorCount;//((EzVarInteger)optionDict.get(MAXIMUM_ERROR_COUNT)).getValue();

        final int width_out =  featureCount;//((EzVarInteger)optionDict.get(FEATURE_COUNT)).getValue();
        final int height_out = in.getSizeY();
        final int t_length_out = in.getSizeT();
        final int channel_scale_out =  groupNames.length;//channel_names.length;
        final int channels_out = in.getSizeC()*channel_scale_out; 
        final int depth_out = in.getSizeZ();
        final DataType dataType_out = outputDataType;//(DataType) optionDict.get(FEATURE_DATA_TYPE);

        // this plug-in processes each channel of each stack in a separate thread
        ExecutorService service = multiThread ? Executors.newFixedThreadPool(cpu) : Executors.newSingleThreadExecutor();
        ArrayList<Future<?>> futures = new ArrayList<Future<?>>(channels * length* depth);

        //Allocate output sequence 
    	for(int t=0; t<t_length_out; ++t) {
			for(int z=0; z<depth_out; ++z) {
				out.setImage(t, z, new IcyBufferedImage(width_out, height_out, channels_out, dataType_out));
			}
		}
        //out.getColorModel().setColorMap(c, in.getColorModel().getColorMap(c), true);

    	
        for (int t = 0; t < length; t++)
        {
        	if(stopFlag)
        		break;
            final int time = t;
            
            final Object out_Z_C_XY = out.getDataXYCZ(time); //[Z][C][XY]
            
            for (int c = 0; c < channels; c++)
            {
            	if(stopFlag)
            		break;
                final int channel = c;
                //set channel name
                for (int co = 0; co < channel_scale_out; co++)
            	{
                	out.setChannelName(channel_scale_out*channel+co, in.getChannelName(c)+ "/"+groupNames[co]);//channel_names[co]);
            	}

                for (int z = 0; z <depth ; z++)
                {
                	if(stopFlag)
                		break;
                        	
                	final Object in_XY = in.getDataXY(time,z, channel);
                	
                	final int stack =z;
                    futures.add(service.submit(new Runnable()
                    {

                        @Override
                        public void run()
                        {
                            int errorCount = 0;
                        	int off = 0;
                        	int offout = 0;
                            for (int y = 0; y < height; y++)
                            {
                                double[] lineToInput = new double[width];
                                
                                if (restrictToROI && rois.size() > 0)
                                {
                                    int nbValues = 0;
                                    
                                    for (int x = 0; x < width; x++, off++)
                                    {
                                        for (ROI roi : rois)
                                        {
                                            if (roi.contains(x, y, stack, time, channel))
                                            {
                                                lineToInput[nbValues++] = Array1DUtil.getValue(in_XY, off, dataType);
                                                break;
                                            }
                                        }
                                    }
                                    
                                    if (nbValues == 0) continue;
                                    
                                    if (nbValues < lineToInput.length) lineToInput = Arrays.copyOf(lineToInput, nbValues);
                                }
                                else
                                {
                                    for (int x = 0; x < width; x++, off++)
                                        lineToInput[x] = Array1DUtil.getValue( in_XY, off, dataType);
                                }
                                
                                double[] lineToOutput = null;//new double[channel_scale_out*width_out]; //[C][X]
                                //int outputLength = channel_scale_out*width_out;
                                try
                                {
                                	lineToOutput = featureFunc.process(lineToInput, new Point5D.Integer(-1,y,stack,time,channel));
		                               if(lineToOutput!= null)
		                               {
		                            	   //if(lineToOutput.length>=outputLength)
		                            	   {
				                                if (restrictToROI && rois.size() > 0)
				                                {
				                                    int nbValues = 0;
				                                    for (int x = 0; x < width_out; x++,offout++)
			                                    	{
					                                    for (int co = 0; co < channel_scale_out; co++)
					                                    {
					                                        for (ROI roi : rois)
					                                        {
					                                            if (roi.contains(x, y, stack, time, channel))
					                                            {
					                                            	
					                                            	Array1DUtil.setValue(((Object[][]) out_Z_C_XY)[stack][channel_scale_out*channel+co], offout, dataType_out,lineToOutput[nbValues++]);
					                                            	
					                                                break;
					                                            }
					                                        }
					                                    }
			                                    	}
				                                    
				                                    if (nbValues == 0) continue;
				                                    
				                                    if (nbValues < lineToOutput.length) lineToOutput = Arrays.copyOf(lineToOutput, nbValues);
				                                }
				                                else
				                                {
				                                	int nbValues = 0;
			                                    	for (int x = 0; x < width_out; x++,offout++)
			                                    	{
			                                    		for (int co = 0; co < channel_scale_out; co++)
			                                    			Array1DUtil.setValue(((Object[][]) out_Z_C_XY)[stack][channel_scale_out*channel+co], offout, dataType_out,lineToOutput[nbValues++]);
			                                    	}
				                                }
		                            	   }
		                               }
                                }
                                catch(Exception e)
                                {
                                	
                                	System.err.println("Error when excuting Extraction Fucntion, t:"+time+" c:"+channel+" z:"+stack+" y:"+y);
                                	
                            		e.printStackTrace();
                            		
                            		
                            		if(errorCount++>=error_exit_count){
                            			MessageDialog.showDialog("Stoped because of error! t:"+time+" c:"+channel+" z:"+stack+" y:"+y,
                        						MessageDialog.ERROR_MESSAGE);
                            			stopFlag = error_exit_count<0?stopFlag:true; //if error_exit_count == -1 then do not stop
                            			return;
                            		}
                                		
                                }   
                               // Array1DUtil.doubleArrayToSafeArray(lineToOutput, out_Z_XY, dataType.isSigned());
                                
                            } // y
                          }
                     }));
                } 
                            
                   
            }
        }
        
        try
        {
            for (Future<?> future : futures)
                future.get();
        }
        catch (InterruptedException e)
        {
            e.printStackTrace();
        }
        catch (ExecutionException e)
        {
            e.printStackTrace();
        }
        
        service.shutdown();
        
        out.updateChannelsBounds(true);
        
    	try{
			featureFunc.batchEnd();
		}
		catch(Exception e){
			System.err.println("Error when excuting batchEnd().");
			e.printStackTrace();
			return null;
		}
        return out;
    }
    /**
     * Performs a Y extraction of the input sequence using the specified algorithm. If the sequence
     * is already 2D, then a copy of the sequence is returned
     * 
     * @param sequence
     *            the sequence to extract
     * @param multiThread
     *            true if the process should be multi-threaded
     * @param restrictToROI
     *            <code>true</code> maps only data located within the sequence ROI,
     *            <code>false</code> maps the entire data set
     * @return the Extracted sequence
     */
    @SuppressWarnings("unchecked")
	public Sequence yExtraction(final Sequence in, boolean multiThread, final boolean restrictToROI)
    {
        final Sequence out = new Sequence(OMEUtil.createOMEMetadata(in.getMetadata()));
        out.setName("Extraction of " + in.getName() +" along Y");
        final FeatureExtractionFunction featureFunc = selectedExtractionFunc;
        if(selectedExtractionFunc==null)
        {
			MessageDialog.showDialog("Extraction Function is not available",
					MessageDialog.ERROR_MESSAGE);
			return null;
        }
        
    	try{
			featureFunc.batchBegin();
		}
		catch(Exception e){
			System.err.println("Error when excuting batchBegin().");
			e.printStackTrace();
			return null;
		}
    	updateFromConfigurations();
    	if(featureCount<1) featureCount = testOutputLength(in,featureFunc)/groupNames.length;
    	
        final List<ROI> rois = in.getROIs();
        int cpu = Runtime.getRuntime().availableProcessors();

        final int depth = in.getSizeZ();
        final int width = in.getSizeX();
        final int height = in.getSizeY();
        final int length = in.getSizeT();
        final int channels = in.getSizeC();
        
        final DataType dataType = in.getDataType_();

		final int error_exit_count = maxErrorCount;//((EzVarInteger)optionDict.get(MAXIMUM_ERROR_COUNT)).getValue();

        final int width_out = in.getSizeX();
        final int height_out =featureCount;// ((EzVarInteger)optionDict.get(FEATURE_COUNT)).getValue();
        final int t_length_out = in.getSizeT();
        final int channel_scale_out =  groupNames.length;//channel_names.length;
        final int channels_out = in.getSizeC()*channel_scale_out; 
        final int depth_out = in.getSizeZ();
        final DataType dataType_out = outputDataType;//(DataType) optionDict.get(FEATURE_DATA_TYPE);

        // this plug-in processes each channel of each stack in a separate thread
        ExecutorService service = multiThread ? Executors.newFixedThreadPool(cpu) : Executors.newSingleThreadExecutor();
        ArrayList<Future<?>> futures = new ArrayList<Future<?>>(channels * length * depth);

        //Allocate output sequence 
    	for(int t=0; t<t_length_out; ++t) {
			for(int z=0; z<depth_out; ++z) {
				out.setImage(t, z, new IcyBufferedImage(width_out, height_out, channels_out, dataType_out));
			}
		}
        //out.getColorModel().setColorMap(c, in.getColorModel().getColorMap(c), true);

        for (int t = 0; t < length; t++)
        {
        	if(stopFlag)
        		break;
            final int time = t;
            final Object out_Z_C_XY = out.getDataXYCZ(time); //[Z][C][XY]
            
            for (int c = 0; c < channels; c++)
            {
            	if(stopFlag)
            		break;
                final int channel = c;
                //set channel name
                for (int co = 0; co < channel_scale_out; co++)
            	{
                	out.setChannelName(channel_scale_out*channel+co, in.getChannelName(c)+ "/"+groupNames[co]);//channel_names[co]);
            	}
                for (int z = 0; z <depth ; z++)
                {
                	if(stopFlag)
                		break;
                	final int stack = z;
                	
                    final Object in_XY = in.getDataXY(time,stack,channel);
                    futures.add(service.submit(new Runnable()
                    {
                        @Override
                        public void run()
                        {
                        	int errorCount = 0;
                            for (int x = 0; x < width; x++)
                            {
                                double[] lineToInput = new double[height];
                                if (restrictToROI && rois.size() > 0)
                                {
                                    int nbValues = 0;
                                    
                                    for (int y = 0; y < height; y++)
                                    {
                                        for (ROI roi : rois)
                                        {
                                            if (roi.contains(x, y, stack, time, channel))
                                            {
                                                lineToInput[nbValues++] = Array1DUtil.getValue(in_XY, y*width+x, dataType);
                                                break;
                                            }
                                        }
                                    }
                                    
                                    if (nbValues == 0) continue;
                                    
                                    if (nbValues < lineToInput.length) lineToInput = Arrays.copyOf(lineToInput, nbValues);
                                }
                                else
                                {
                                    for (int y = 0; y < height; y++)
                                        lineToInput[y] = Array1DUtil.getValue( in_XY, y*width+x, dataType);
                                }
                                
                                double[] lineToOutput = null;//new double[channel_scale_out*height_out]; //[C][X]
                                //int outputLength = channel_scale_out*height_out;
                                try
                                {
                                	lineToOutput = featureFunc.process(lineToInput, new Point5D.Integer(x,-1,stack,time,channel));
                                	
		                               if(lineToOutput!= null)
		                               {
		                            	  // if(lineToOutput.length>=outputLength)
		                            	   {
				                                if (restrictToROI && rois.size() > 0)
				                                {
				                                    int nbValues = 0;
				                                    for (int y = 0; y < height_out; y++)
			                                    	{
					                                    for (int co = 0; co < channel_scale_out; co++)
					                                    {
					                                        for (ROI roi : rois)
					                                        {
					                                            if (roi.contains(x, y, stack, time, channel))
					                                            {
					                                            	
					                                            	Array1DUtil.setValue(((Object[][]) out_Z_C_XY)[stack][channel_scale_out*channel+co], y*width_out+x, dataType_out,lineToOutput[nbValues++]);
					                                            	
					                                                break;
					                                            }
					                                        }
					                                    }
			                                    	}
				                                    
				                                    if (nbValues == 0) continue;
				                                    
				                                    if (nbValues < lineToOutput.length) lineToOutput = Arrays.copyOf(lineToOutput, nbValues);
				                                }
				                                else
				                                {
				                                	int nbValues = 0;
			                                    	for (int y = 0; y < height_out; y++)
			                                    	{
			                                    		for (int co = 0; co < channel_scale_out; co++)
			                                    			Array1DUtil.setValue(((Object[][]) out_Z_C_XY)[stack][channel_scale_out*channel+co],  y*width_out+x, dataType_out,lineToOutput[nbValues++]);
			                                    	}
				                                }
		                            	   }
		                               }
                                }
                                catch(Exception e)
                                {
                                	
                                	System.err.println("Error when excuting Extraction Fucntion, t:"+time+" c:"+channel+" z:"+stack+" x:"+x);
                                	
                            		e.printStackTrace();
                            		
                            		
                            		if(errorCount++>=error_exit_count){
                            			MessageDialog.showDialog("Stoped because of error! t:"+time+" c:"+channel+" z:"+stack+" x:"+x,
                        						MessageDialog.ERROR_MESSAGE);
                            			stopFlag = error_exit_count<0?stopFlag:true; //if error_exit_count == -1 then do not stop
                            			return;
                            		}
                                		
                                }   
                               // Array1DUtil.doubleArrayToSafeArray(lineToOutput, out_Z_XY, dataType.isSigned());
                                
                            } // x
                        }
                    }));
                 } 
            }
        }
        
        try
        {
            for (Future<?> future : futures)
                future.get();
        }
        catch (InterruptedException e)
        {
            e.printStackTrace();
        }
        catch (ExecutionException e)
        {
            e.printStackTrace();
        }
        
        service.shutdown();
        
        out.updateChannelsBounds(true);
    	try{
			featureFunc.batchEnd();
		}
		catch(Exception e){
			System.err.println("Error when excuting batchEnd().");
			e.printStackTrace();
			return null;
		}
        return out;
    }
    /**
     * Performs a T extraction of the input sequence using the specified algorithm. If the sequence
     * has only one time point, then a copy of the sequence is returned
     * 
     * @param sequence
     *            the sequence to extract
     * @param multiThread
     *            true if the process should be multi-threaded
     * @return the Extracted sequence
     */
    public Sequence tExtraction(final Sequence in, boolean multiThread)
    {
        return tExtraction(in, multiThread, false);
    }
    
    /**
     * Performs a T extraction of the input sequence using the specified algorithm. If the sequence
     * has only one time point, then a copy of the sequence is returned
     * 
     * @param sequence
     *            the sequence to extract
     * @param multiThread
     *            true if the process should be multi-threaded
     * @param restrictToROI
     *            <code>true</code> maps only data located within the sequence ROI,
     *            <code>false</code> map the entire data set
     * @return the Extracted sequence
     */
    @SuppressWarnings("unchecked")
	public Sequence tExtraction(final Sequence in, boolean multiThread, final boolean restrictToROI)
    {
    	
        final Sequence out = new Sequence(OMEUtil.createOMEMetadata(in.getMetadata()));
        out.setName("Extraction of " + in.getName() +" along T");
        final FeatureExtractionFunction featureFunc = selectedExtractionFunc;
        if(selectedExtractionFunc==null)
        {
			MessageDialog.showDialog("Extraction Function is not available",
					MessageDialog.ERROR_MESSAGE);
			return null;
        }
        
    	try{
			featureFunc.batchBegin();
		}
		catch(Exception e){
			System.err.println("Error when excuting batchBegin().");
			e.printStackTrace();
			return null;
		}
    	updateFromConfigurations();
    	if(featureCount<1) featureCount = testOutputLength(in,featureFunc)/groupNames.length;
    	
        final List<ROI> rois = in.getROIs();
        int cpu = Runtime.getRuntime().availableProcessors();


        
        final DataType dataType = in.getDataType_();
        

		final int error_exit_count = maxErrorCount;//((EzVarInteger)optionDict.get(MAXIMUM_ERROR_COUNT)).getValue(); 
		
		//Convert to Z stack
        SequenceUtil.adjustZT(in,in.getSizeT(),in.getSizeZ(),true);
        
        final int depth = in.getSizeZ();
        final int width = in.getSizeX();
        final int height = in.getSizeY();
        final int length = in.getSizeT();
        final int channels = in.getSizeC();
        
        final int width_out = in.getSizeX();
        final int height_out = in.getSizeY();
        final int t_length_out = in.getSizeT();
        final int channel_scale_out =  groupNames.length;//channel_names.length;
        final int channels_out = in.getSizeC()*channel_scale_out; 
        final int depth_out = featureCount;//((EzVarInteger)optionDict.get(FEATURE_COUNT)).getValue();
        final DataType dataType_out = outputDataType;//(DataType) optionDict.get(FEATURE_DATA_TYPE);


        // this plug-in processes each channel of each stack in a separate thread
        ExecutorService service = multiThread ? Executors.newFixedThreadPool(cpu) : Executors.newSingleThreadExecutor();
        ArrayList<Future<?>> futures = new ArrayList<Future<?>>(channels*length *height *width);

        //Allocate output sequence 
    	for(int t=0; t<t_length_out; ++t) {
			for(int z=0; z<depth_out; ++z) {
				out.setImage(t, z, new IcyBufferedImage(width_out, height_out, channels_out, dataType_out));
			}
		}
        //out.getColorModel().setColorMap(c, in.getColorModel().getColorMap(c), true);

        for (int t = 0; t < length; t++)
        {
        	if(stopFlag)
        		break;
            final int time = t;
            
            final Object out_Z_C_XY = out.getDataXYCZ(time); //[Z][C][XY]
            
            for (int c = 0; c < channels; c++)
            {
            	if(stopFlag)
            		break;
                final int channel = c;
                //set channel name
                for (int co = 0; co < channel_scale_out; co++)
            	{
                	out.setChannelName(channel_scale_out*channel+co, in.getChannelName(c)+ "/"+groupNames[co]);//channel_names[co]);
            	}

                final Object in_Z_XY = in.getDataXYZ(time, channel);
                int off = 0;
                for (int y = 0; y < height; y++)
                {
                	if(stopFlag)
                		break;
                    for (int x = 0; x < width; x++, off++)
                    {
                    	if(stopFlag)
                    		break;
                    	final int offf=off;
                    	final int yy=y;
                    	final int xx=x;
                        futures.add(service.submit(new Runnable()
                        {
                            @Override
                            public void run()
                            {
                            	int errorCount = 0;
                                double[] lineToInput = new double[depth];
                                
                                if (restrictToROI && rois.size() > 0)
                                {
                                    int nbValues = 0;
                                    
                                    for (int z = 0; z < depth; z++)
                                    {
                                        for (ROI roi : rois)
                                        {
                                            if (roi.contains(xx, yy, z, time, channel))
                                            {
                                                lineToInput[nbValues++] = Array1DUtil.getValue(((Object[]) in_Z_XY)[z], offf, dataType);
                                                break;
                                            }
                                        }
                                    }
                                    
                                    if (nbValues == 0) return;
                                    
                                    if (nbValues < lineToInput.length) lineToInput = Arrays.copyOf(lineToInput, nbValues);
                                }
                                else
                                {
                                    for (int z = 0; z < depth; z++)
                                        lineToInput[z] = Array1DUtil.getValue(((Object[]) in_Z_XY)[z], offf, dataType);
                                }
                                
                                double[] lineToOutput = null;//new double[channel_scale_out*depth_out]; //[C][Z]
                                //int outputLength = channel_scale_out*depth_out;
                                try
                                {
                                	lineToOutput = featureFunc.process(lineToInput, new Point5D.Integer(xx,yy,time,-1,channel));
                                	
		                               if(lineToOutput != null) //swap t and z
		                               {
		                            	   //if(lineToOutput.length>=outputLength)
		                            	   {
				                                if (restrictToROI && rois.size() > 0)
				                                {
				                                    int nbValues = 0;
				                                    for (int co = 0; co < channel_scale_out; co++)
			                                    	{
					                                    for (int z = 0; z < depth_out; z++)
					                                    {
					                                        for (ROI roi : rois)
					                                        {
					                                            if (roi.contains(xx, yy, z, time, channel))
					                                            {
					                                            	
					                                            	Array1DUtil.setValue(((Object[][]) out_Z_C_XY)[z][channel_scale_out*channel+co], offf, dataType_out,lineToOutput[nbValues++]);
					                                            	
					                                                break;
					                                            }
					                                        }
					                                    }
			                                    	}
				                                    
				                                    if (nbValues == 0) return;
				                                    
				                                    if (nbValues < lineToOutput.length) lineToOutput = Arrays.copyOf(lineToOutput, nbValues);
				                                }
				                                else
				                                {
				                                	int nbValues = 0;
			                                    	for (int co = 0; co < channel_scale_out; co++)
			                                    	{
			                                    		for (int z = 0; z < depth_out; z++)
			                                    			Array1DUtil.setValue(((Object[][]) out_Z_C_XY)[z][channel_scale_out*channel+co], offf, dataType_out,lineToOutput[nbValues++]);
			                                    	}
				                                }
		                            	   }
		                               }
                                }
                                catch(Exception e)
                                {
                                	
                                	System.err.println("Error when excuting Extraction Fucntion, z:"+time+" c:"+channel+" x:"+xx+" y:"+yy);
                                	
                            		e.printStackTrace();
                            		
                            		
                            		if(errorCount++>=error_exit_count){
                            			MessageDialog.showDialog("Stoped because of error! z:"+time+" c:"+channel+" x:"+xx+" y:"+yy,
                        						MessageDialog.ERROR_MESSAGE);
                            			stopFlag = error_exit_count<0?stopFlag:true; //if error_exit_count == -1 then do not stop
                            			return;
                            		}
                                		

                                }   
                               // Array1DUtil.doubleArrayToSafeArray(lineToOutput, out_Z_XY, dataType.isSigned());
                            }
                        }));
                    } // x
                } // y
            }
        }
        
        try
        {
            for (Future<?> future : futures)
                future.get();
        }
        catch (InterruptedException e)
        {
            e.printStackTrace();
        }
        catch (ExecutionException e)
        {
            e.printStackTrace();
        }
        
        service.shutdown();
        
        out.updateChannelsBounds(true);
        
    	//Convert Z to T 
        SequenceUtil.adjustZT(in,in.getSizeT(),in.getSizeZ(),true);
        
        SequenceUtil.adjustZT(out,out.getSizeT(),out.getSizeZ(),true);
    	try{
			featureFunc.batchEnd();
		}
		catch(Exception e){
			System.err.println("Error when excuting batchEnd().");
			e.printStackTrace();
			return null;
		}
        return out;
    }
    @Override
	public void stopExecution()
	{
		stopFlag = true;
	}
    
    @Override
    public void declareInput(VarList inputMap)
    {   if(inputMap_==null)
        	inputMap_ = inputMap;
    	buildFeatureFuncList();
        inputMap.add("input", input.getVariable());
        inputMap.add("extract direction", extractDir.getVariable());
        inputMap.add("extraction function", featureFuncVar.getVariable());
        inputMap.add("restrict to ROI", restrictToROI.getVariable());

    }
    
    @Override
    public void declareOutput(VarList outputMap)
    {
        outputMap.add("Output Sequence", output.getVariable());
    }
    
}
