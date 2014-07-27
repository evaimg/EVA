package plugins.oeway.mapfun;

import icy.gui.dialog.MessageDialog;
import icy.image.IcyBufferedImage;
import icy.plugin.PluginDescriptor;
import icy.plugin.PluginLoader;
import icy.plugin.abstract_.Plugin;
import icy.roi.ROI;
import icy.sequence.Sequence;
import icy.type.DataType;
import icy.type.collection.array.Array1DUtil;
import icy.util.ClassUtil;
import icy.util.OMEUtil;
import icy.util.StringUtil;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

import plugins.adufour.blocks.lang.Block;
import plugins.adufour.blocks.util.VarList;
import plugins.adufour.ezplug.EzGroup;
import plugins.adufour.ezplug.EzPlug;
import plugins.adufour.ezplug.EzVar;
import plugins.adufour.ezplug.EzVarBoolean;
import plugins.adufour.ezplug.EzVarEnum;
import plugins.adufour.ezplug.EzVarListener;
import plugins.adufour.ezplug.EzVarSequence;
import plugins.adufour.ezplug.EzVarText;
import plugins.adufour.vars.lang.VarSequence;

public class MapFun extends EzPlug implements Block
{
    public enum MapDirection
    {
        X,Y,Z, T
    }
    
    private final EzVarSequence                  input          = new EzVarSequence("Input");
    
    private final EzVarEnum<MapDirection> mapDir  = new EzVarEnum<MapFun.MapDirection>("Map along", MapDirection.values(), MapDirection.Z);
    
    private final EzVarBoolean                   restrictToROI  = new EzVarBoolean("Restrict to ROI", false);
    
    private final VarSequence                    output         = new VarSequence("Maped sequence", null);
    
    private EzGroup mapFuncOptions = new EzGroup("Options");
    
    private HashMap<String,Class<? extends MapFunction>> pluginList ;
    
    EzVarText mapFuncVar;
    
    public MapFunction selectedMapFunc;

	
    
    @Override
    protected void initialize()
    {
    	pluginList = getPluginList();
    	
    	String tmp[] = new String[pluginList.size()];
    	int i=0;
    	for (Iterator<String> iter = pluginList.keySet().iterator(); iter.hasNext();) {
    		tmp[i++]=(String) iter.next();
    	}

    	mapFuncVar = new EzVarText("Map Function", tmp, 0, false);
        
        addEzComponent(input);
        addEzComponent(mapDir);
        addEzComponent(mapFuncVar); 
        addEzComponent(mapFuncOptions);
        restrictToROI.setToolTipText("Check this option to map only the intensity data contained within the sequence ROI");
        addEzComponent(restrictToROI);
    	

    	
        mapFuncVar.addVarChangeListener(new EzVarListener<String>(){
			@Override
			public void variableChanged(EzVar<String> source, String newValue) {
				try {
					selectedMapFunc = pluginList.get(mapFuncVar.getValue()).newInstance();
					mapFuncOptions.components.clear();
					selectedMapFunc.initialize(mapFuncOptions);
			    	
				} catch (InstantiationException e1) {
					e1.printStackTrace();
					return ;
				} catch (IllegalAccessException e1) {
					e1.printStackTrace();
					return ;
				}
				
			}
        });
    }
    
    @Override
    protected void execute()
    {
        switch (mapDir.getValue())
        {
	        case X:
	            output.setValue(xMap(input.getValue(true), true, restrictToROI.getValue()));
	        break;
	        case Y:
	            output.setValue(yMap(input.getValue(true), true, restrictToROI.getValue()));
	        break;
            case T:
                output.setValue(tMap(input.getValue(true), true, restrictToROI.getValue()));
            break;
            case Z:
                output.setValue(zMap(input.getValue(true), true, restrictToROI.getValue()));
            break;
            default:
                throw new UnsupportedOperationException("Map along " + mapDir.getValue() + " not supported");
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
    
    public  HashMap<String,Class<? extends MapFunction>> getPluginList()
    {
    	 HashMap<String,Class<? extends MapFunction>> pl =  new HashMap<String,Class<? extends MapFunction>>();
    	 ArrayList<PluginDescriptor> plugins = PluginLoader.getPlugins(MapFunction.class, true, false, false);
         
         if (plugins.size() == 0)
         {
             return pl;
         }
         
         for (PluginDescriptor descriptor : plugins)
         {
             Class<? extends Plugin> clazz = descriptor.getPluginClass();
             try
             {
                 final Class<? extends MapFunction> funcClass = clazz.asSubclass(MapFunction.class);

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
    

    
    /**
     * Performs a Z map of the input sequence using the specified algorithm. If the sequence
     * is already 2D, then a copy of the sequence is returned
     * 
     * @param sequence
     *            the sequence to map
     * @param map
     *            the type of map to perform (see {@link MapType} enumeration)
     * @param multiThread
     *            true if the process should be multi-threaded
     * @return the maped sequence
     */
    public Sequence zMap(final Sequence in, boolean multiThread)
    {
        return zMap(in, multiThread, false);
    }
    
    /**
     * Performs a Z map of the input sequence using the specified algorithm. If the sequence
     * is already 2D, then a copy of the sequence is returned
     * 
     * @param sequence
     *            the sequence to map
     * @param map
     *            the type of map to perform (see {@link MapType} enumeration)
     * @param multiThread
     *            true if the process should be multi-threaded
     * @param restrictToROI
     *            <code>true</code> maps only data located within the sequence ROI,
     *            <code>false</code> maps the entire data set
     * @return the maped sequence
     */
    @SuppressWarnings("unchecked")
	public Sequence zMap(final Sequence in, boolean multiThread, final boolean restrictToROI)
    {
        final Sequence out = new Sequence(OMEUtil.createOMEMetadata(in.getMetadata()));
        out.setName("Z map of " + in.getName());
        final MapFunction mapFunc = selectedMapFunc;
        if(selectedMapFunc==null)
        {
			MessageDialog.showDialog("Map Function is not available",
					MessageDialog.ERROR_MESSAGE);
			return null;
        }
        final List<ROI> rois = in.getROIs();
        int cpu = Runtime.getRuntime().availableProcessors();

        final int depth = in.getSizeZ();
        final int width = in.getSizeX();
        final int height = in.getSizeY();
        final int length = in.getSizeT();
        final int channels = in.getSizeC();
        
        final DataType dataType = in.getDataType_();
        
		HashMap<String,Object> configurations = new HashMap<String,Object>();
        configurations.put("SizeX", in.getSizeX());
    	configurations.put("SizeY", in.getSizeY());
    	configurations.put("SizeT", in.getSizeT());
    	configurations.put("SizeZ", in.getSizeZ());
    	configurations.put("SizeC", in.getSizeC());
    	configurations.put("OutputDataType", in.getDataType_());
    	
    	configurations.put("MaximumErrorCount", 1);
    	String[] channelNames = new String[]{""};
    	configurations.put("OutputChannels",channelNames );
    	configurations.put("OutputLength", 1);
		
		try{
			mapFunc.configure(configurations);
		}
		catch(Exception e){
			System.err.println("Error when excuting config");
			e.printStackTrace();
			return null;
		}

		final int error_exit_count = (int) configurations.get("MaximumErrorCount");

		final String[] channel_names = (String[])configurations.get("OutputChannels");
        final int width_out = in.getSizeX();
        final int height_out = in.getSizeY();
        final int t_length_out = in.getSizeT();
        final int channel_scale_out = channel_names.length;
        final int channels_out = in.getSizeC()*channel_scale_out; 
        final int z_length_out = (int) configurations.get("OutputLength");
        final int depth_out = z_length_out;
        final DataType dataType_out = (DataType) configurations.get("OutputDataType");

        // this plug-in processes each channel of each stack in a separate thread
        ExecutorService service = multiThread ? Executors.newFixedThreadPool(cpu) : Executors.newSingleThreadExecutor();
        ArrayList<Future<?>> futures = new ArrayList<Future<?>>(channels * length);

        //Allocate output sequence 
    	for(int t=0; t<t_length_out; ++t) {
			for(int z=0; z<z_length_out; ++z) {
				out.setImage(t, z, new IcyBufferedImage(width_out, height_out, channels_out, dataType_out));
			}
		}
        //out.getColorModel().setColorMap(c, in.getColorModel().getColorMap(c), true);

        for (int t = 0; t < length; t++)
        {
            final int time = t;
            
            final Object out_Z_C_XY = out.getDataXYCZ(time); //[Z][C][XY]
            
            for (int c = 0; c < channels; c++)
            {
                final int channel = c;
                //set channel name
                for (int co = 0; co < channel_scale_out; co++)
            	{
                	out.setChannelName(channel_scale_out*channel+co, in.getChannelName(c)+ "/"+channel_names[co]);
            	}
                futures.add(service.submit(new Runnable()
                {
                    @Override
                    public void run()
                    {
                        Object in_Z_XY = in.getDataXYZ(time, channel);
                        
                        int errorCount = 0;
                        int off = 0;
                        for (int y = 0; y < height; y++)
                        {
                        	
                            for (int x = 0; x < width; x++, off++)
                            {
                            	
                                double[] lineToInput = new double[depth];
                                
                                if (restrictToROI && rois.size() > 0)
                                {
                                    int nbValues = 0;
                                    
                                    for (int z = 0; z < depth; z++)
                                    {
                                        for (ROI roi : rois)
                                        {
                                            if (roi.contains(x, y, z, time, channel))
                                            {
                                                lineToInput[nbValues++] = Array1DUtil.getValue(((Object[]) in_Z_XY)[z], off, dataType);
                                                break;
                                            }
                                        }
                                    }
                                    
                                    if (nbValues == 0) continue;
                                    
                                    if (nbValues < lineToInput.length) lineToInput = Arrays.copyOf(lineToInput, nbValues);
                                }
                                else
                                {
                                    for (int z = 0; z < depth; z++)
                                        lineToInput[z] = Array1DUtil.getValue(((Object[]) in_Z_XY)[z], off, dataType);
                                }
                                
                                double[] lineToOutput = new double[channel_scale_out*depth_out]; //[C][Z]
                               
                                try
                                {
		                               if(mapFunc.process(lineToInput,lineToOutput))
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
				                                            if (roi.contains(x, y, z, time, channel))
				                                            {
				                                            	
				                                            	Array1DUtil.setValue(((Object[][]) out_Z_C_XY)[z][channel_scale_out*channel+co], off, dataType_out,lineToOutput[nbValues++]);
				                                            	
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
		                                    	for (int co = 0; co < channel_scale_out; co++)
		                                    	{
		                                    		for (int z = 0; z < depth_out; z++)
		                                    			Array1DUtil.setValue(((Object[][]) out_Z_C_XY)[z][channel_scale_out*channel+co], off, dataType_out,lineToOutput[nbValues++]);
		                                    	}
			                                }
		                               }
                                }
                                catch(Exception e)
                                {
                                	
                                	System.err.println("Error when excuting Map Fucntion, t:"+time+" c:"+channel+" x:"+x+" y:"+y);
                                	
                            		e.printStackTrace();
                            		
                            		
                            		if(errorCount++>=error_exit_count){
                            			MessageDialog.showDialog("Stoped because of error! t:"+time+" c:"+channel+" x:"+x+" y:"+y,
                        						MessageDialog.ERROR_MESSAGE);
                            			return;
                            		}
                                		

                                }   
                               // Array1DUtil.doubleArrayToSafeArray(lineToOutput, out_Z_XY, dataType.isSigned());
                                
                            } // x
                        } // y
                    }
                }));
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
        return out;
    }
    /**
     * Performs a X map of the input sequence using the specified algorithm. If the sequence
     * is already 2D, then a copy of the sequence is returned
     * 
     * @param sequence
     *            the sequence to map
     * @param map
     *            the type of map to perform (see {@link MapType} enumeration)
     * @param multiThread
     *            true if the process should be multi-threaded
     * @param restrictToROI
     *            <code>true</code> maps only data located within the sequence ROI,
     *            <code>false</code> maps the entire data set
     * @return the maped sequence
     */
    @SuppressWarnings("unchecked")
	public Sequence xMap(final Sequence in, boolean multiThread, final boolean restrictToROI)
    {
        final Sequence out = new Sequence(OMEUtil.createOMEMetadata(in.getMetadata()));
        out.setName("X map of " + in.getName());
        final MapFunction mapFunc = selectedMapFunc;
        if(selectedMapFunc==null)
        {
			MessageDialog.showDialog("Map Function is not available",
					MessageDialog.ERROR_MESSAGE);
			return null;
        }
        final List<ROI> rois = in.getROIs();
        int cpu = Runtime.getRuntime().availableProcessors();

        final int depth = in.getSizeZ();
        final int width = in.getSizeX();
        final int height = in.getSizeY();
        final int length = in.getSizeT();
        final int channels = in.getSizeC();
        
        final DataType dataType = in.getDataType_();
        
    	HashMap<String,Object> configurations = new HashMap<String,Object>();
    	
        configurations.put("SizeX", in.getSizeX());
    	configurations.put("SizeY", in.getSizeY());
    	configurations.put("SizeT", in.getSizeT());
    	configurations.put("SizeZ", in.getSizeZ());
    	configurations.put("SizeC", in.getSizeC());
    	configurations.put("OutputDataType", in.getDataType_());
    	
    	configurations.put("MaximumErrorCount", 1);
    	String[] channelNames = new String[]{""};
    	configurations.put("OutputChannels",channelNames );
    	configurations.put("OutputLength", 1);
		
		try{
			mapFunc.configure(configurations);
		}
		catch(Exception e){
			System.err.println("Error when excuting config");
			e.printStackTrace();
			return null;
		}

		final int error_exit_count = (int) configurations.get("MaximumErrorCount");

		final String[] channel_names = (String[])configurations.get("OutputChannels");
        final int width_out = (int) configurations.get("OutputLength");
        final int height_out = in.getSizeY();
        final int t_length_out = in.getSizeT();
        final int channel_scale_out = channel_names.length;
        final int channels_out = in.getSizeC()*channel_scale_out; 
        final int z_length_out = in.getSizeZ();
        final int depth_out = z_length_out;
        final DataType dataType_out = (DataType) configurations.get("OutputDataType");

        // this plug-in processes each channel of each stack in a separate thread
        ExecutorService service = multiThread ? Executors.newFixedThreadPool(cpu) : Executors.newSingleThreadExecutor();
        ArrayList<Future<?>> futures = new ArrayList<Future<?>>(channels * length);

        //Allocate output sequence 
    	for(int t=0; t<t_length_out; ++t) {
			for(int z=0; z<z_length_out; ++z) {
				out.setImage(t, z, new IcyBufferedImage(width_out, height_out, channels_out, dataType_out));
			}
		}
        //out.getColorModel().setColorMap(c, in.getColorModel().getColorMap(c), true);

        for (int t = 0; t < length; t++)
        {
            final int time = t;
            
            final Object out_Z_C_XY = out.getDataXYCZ(time); //[Z][C][XY]
            
            for (int c = 0; c < channels; c++)
            {
                final int channel = c;
                //set channel name
                for (int co = 0; co < channel_scale_out; co++)
            	{
                	out.setChannelName(channel_scale_out*channel+co, in.getChannelName(c)+ "/"+channel_names[co]);
            	}
                futures.add(service.submit(new Runnable()
                {
                    @Override
                    public void run()
                    {
                        Object in_Z_XY = in.getDataXYZ(time, channel);
                        
                        int errorCount = 0;
                        
                        for (int z = 0; z <depth ; z++)
                        {
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
                                            if (roi.contains(x, y, z, time, channel))
                                            {
                                                lineToInput[nbValues++] = Array1DUtil.getValue(((Object[]) in_Z_XY)[z], off, dataType);
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
                                        lineToInput[x] = Array1DUtil.getValue(((Object[]) in_Z_XY)[z], off, dataType);
                                }
                                
                                double[] lineToOutput = new double[channel_scale_out*width_out]; //[C][X]
                               
                                try
                                {
		                               if(mapFunc.process(lineToInput,lineToOutput))
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
				                                            if (roi.contains(x, y, z, time, channel))
				                                            {
				                                            	
				                                            	Array1DUtil.setValue(((Object[][]) out_Z_C_XY)[z][channel_scale_out*channel+co], offout, dataType_out,lineToOutput[nbValues++]);
				                                            	
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
		                                    			Array1DUtil.setValue(((Object[][]) out_Z_C_XY)[z][channel_scale_out*channel+co], offout, dataType_out,lineToOutput[nbValues++]);
		                                    	}
			                                }
		                               }
                                }
                                catch(Exception e)
                                {
                                	
                                	System.err.println("Error when excuting Map Fucntion, t:"+time+" c:"+channel+" z:"+z+" y:"+y);
                                	
                            		e.printStackTrace();
                            		
                            		
                            		if(errorCount++>=error_exit_count){
                            			MessageDialog.showDialog("Stoped because of error! t:"+time+" c:"+channel+" z:"+z+" y:"+y,
                        						MessageDialog.ERROR_MESSAGE);
                            			return;
                            		}
                                		
                                }   
                               // Array1DUtil.doubleArrayToSafeArray(lineToOutput, out_Z_XY, dataType.isSigned());
                                
                            } // x
                        } // y
                    }
                }));
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
        return out;
    }
    /**
     * Performs a X map of the input sequence using the specified algorithm. If the sequence
     * is already 2D, then a copy of the sequence is returned
     * 
     * @param sequence
     *            the sequence to map
     * @param map
     *            the type of map to perform (see {@link MapType} enumeration)
     * @param multiThread
     *            true if the process should be multi-threaded
     * @param restrictToROI
     *            <code>true</code> maps only data located within the sequence ROI,
     *            <code>false</code> maps the entire data set
     * @return the maped sequence
     */
    @SuppressWarnings("unchecked")
	public Sequence yMap(final Sequence in, boolean multiThread, final boolean restrictToROI)
    {
        final Sequence out = new Sequence(OMEUtil.createOMEMetadata(in.getMetadata()));
        out.setName("Y map of " + in.getName());
        final MapFunction mapFunc = selectedMapFunc;
        if(selectedMapFunc==null)
        {
			MessageDialog.showDialog("Map Function is not available",
					MessageDialog.ERROR_MESSAGE);
			return null;
        }
        final List<ROI> rois = in.getROIs();
        int cpu = Runtime.getRuntime().availableProcessors();

        final int depth = in.getSizeZ();
        final int width = in.getSizeX();
        final int height = in.getSizeY();
        final int length = in.getSizeT();
        final int channels = in.getSizeC();
        
        final DataType dataType = in.getDataType_();
        
    	HashMap<String,Object> configurations = new HashMap<String,Object>();
    	
        configurations.put("SizeX", in.getSizeX());
    	configurations.put("SizeY", in.getSizeY());
    	configurations.put("SizeT", in.getSizeT());
    	configurations.put("SizeZ", in.getSizeZ());
    	configurations.put("SizeC", in.getSizeC());
    	configurations.put("OutputDataType", in.getDataType_());
    	
    	configurations.put("MaximumErrorCount", 1);
    	String[] channelNames = new String[]{""};
    	configurations.put("OutputChannels",channelNames );
    	configurations.put("OutputLength", 1);
		
		try{
			mapFunc.configure(configurations);
		}
		catch(Exception e){
			System.err.println("Error when excuting config");
			e.printStackTrace();
			return null;
		}

		final int error_exit_count = (int) configurations.get("MaximumErrorCount");

		final String[] channel_names = (String[])configurations.get("OutputChannels");
        final int width_out = in.getSizeX();
        final int height_out = (int) configurations.get("OutputLength");
        final int t_length_out = in.getSizeT();
        final int channel_scale_out = channel_names.length;
        final int channels_out = in.getSizeC()*channel_scale_out; 
        final int z_length_out = in.getSizeZ();
        final int depth_out = z_length_out;
        final DataType dataType_out = (DataType) configurations.get("OutputDataType");

        // this plug-in processes each channel of each stack in a separate thread
        ExecutorService service = multiThread ? Executors.newFixedThreadPool(cpu) : Executors.newSingleThreadExecutor();
        ArrayList<Future<?>> futures = new ArrayList<Future<?>>(channels * length);

        //Allocate output sequence 
    	for(int t=0; t<t_length_out; ++t) {
			for(int z=0; z<z_length_out; ++z) {
				out.setImage(t, z, new IcyBufferedImage(width_out, height_out, channels_out, dataType_out));
			}
		}
        //out.getColorModel().setColorMap(c, in.getColorModel().getColorMap(c), true);

        for (int t = 0; t < length; t++)
        {
            final int time = t;
            
            final Object out_Z_C_XY = out.getDataXYCZ(time); //[Z][C][XY]
            
            for (int c = 0; c < channels; c++)
            {
                final int channel = c;
                //set channel name
                for (int co = 0; co < channel_scale_out; co++)
            	{
                	out.setChannelName(channel_scale_out*channel+co, in.getChannelName(c)+ "/"+channel_names[co]);
            	}
                futures.add(service.submit(new Runnable()
                {
                    @Override
                    public void run()
                    {
                        Object in_Z_XY = in.getDataXYZ(time, channel);
                        
                        int errorCount = 0;
                        
                        for (int z = 0; z <depth ; z++)
                        {
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
                                            if (roi.contains(x, y, z, time, channel))
                                            {
                                                lineToInput[nbValues++] = Array1DUtil.getValue(((Object[]) in_Z_XY)[z], y*width+x, dataType);
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
                                        lineToInput[y] = Array1DUtil.getValue(((Object[]) in_Z_XY)[z], y*width+x, dataType);
                                }
                                
                                double[] lineToOutput = new double[channel_scale_out*height_out]; //[C][X]
                               
                                try
                                {
		                               if(mapFunc.process(lineToInput,lineToOutput))
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
				                                            if (roi.contains(x, y, z, time, channel))
				                                            {
				                                            	
				                                            	Array1DUtil.setValue(((Object[][]) out_Z_C_XY)[z][channel_scale_out*channel+co], y*width_out+x, dataType_out,lineToOutput[nbValues++]);
				                                            	
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
		                                    			Array1DUtil.setValue(((Object[][]) out_Z_C_XY)[z][channel_scale_out*channel+co],  y*width_out+x, dataType_out,lineToOutput[nbValues++]);
		                                    	}
			                                }
		                               }
                                }
                                catch(Exception e)
                                {
                                	
                                	System.err.println("Error when excuting Map Fucntion, t:"+time+" c:"+channel+" z:"+z+" x:"+x);
                                	
                            		e.printStackTrace();
                            		
                            		
                            		if(errorCount++>=error_exit_count){
                            			MessageDialog.showDialog("Stoped because of error! t:"+time+" c:"+channel+" z:"+z+" x:"+x,
                        						MessageDialog.ERROR_MESSAGE);
                            			return;
                            		}
                                		
                                }   
                               // Array1DUtil.doubleArrayToSafeArray(lineToOutput, out_Z_XY, dataType.isSigned());
                                
                            } // x
                        } // y
                    }
                }));
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
        return out;
    }
    /**
     * Performs a T map of the input sequence using the specified algorithm. If the sequence
     * has only one time point, then a copy of the sequence is returned
     * 
     * @param sequence
     *            the sequence to map
     * @param map
     *            the type of map to perform (see {@link MapType} enumeration)
     * @param multiThread
     *            true if the process should be multi-threaded
     * @return the maped sequence
     */
    public Sequence tMap(final Sequence in, boolean multiThread)
    {
        return tMap(in, multiThread, false);
    }
    
    /**
     * Performs a T map of the input sequence using the specified algorithm. If the sequence
     * has only one time point, then a copy of the sequence is returned
     * 
     * @param sequence
     *            the sequence to map
     * @param map
     *            the type of map to perform (see {@link MapType} enumeration)
     * @param multiThread
     *            true if the process should be multi-threaded
     * @param restrictToROI
     *            <code>true</code> maps only data located within the sequence ROI,
     *            <code>false</code> map the entire data set
     * @return the maped sequence
     */
    @SuppressWarnings("unchecked")
	public Sequence tMap(final Sequence in, boolean multiThread, final boolean restrictToROI)
    {
        final Sequence out = new Sequence(OMEUtil.createOMEMetadata(in.getMetadata()));
        out.setName("Z map of " + in.getName());
        final MapFunction mapFunc = selectedMapFunc;
        if(selectedMapFunc==null)
        {
			MessageDialog.showDialog("Map Function is not available",
					MessageDialog.ERROR_MESSAGE);
			return null;
        }
        final List<ROI> rois = in.getROIs();
        int cpu = Runtime.getRuntime().availableProcessors();

        final int depth = in.getSizeZ();
        final int width = in.getSizeX();
        final int height = in.getSizeY();
        final int length = in.getSizeT();
        final int channels = in.getSizeC();
        
        final DataType dataType = in.getDataType_();
        
		HashMap<String,Object> configurations = new HashMap<String,Object>();
        configurations.put("SizeX", in.getSizeX());
    	configurations.put("SizeY", in.getSizeY());
    	configurations.put("SizeT", in.getSizeT());
    	configurations.put("SizeZ", in.getSizeZ());
    	configurations.put("SizeC", in.getSizeC());
    	configurations.put("OutputDataType", in.getDataType_());
    	
    	configurations.put("MaximumErrorCount", 1);
    	String[] channelNames = new String[]{""};
    	configurations.put("OutputChannels",channelNames );
    	configurations.put("OutputLength", 1);
		
		try{
			mapFunc.configure(configurations);
		}
		catch(Exception e){
			System.err.println("Error when excuting config");
			e.printStackTrace();
			return null;
		}

		final int error_exit_count = (int) configurations.get("MaximumErrorCount");

		final String[] channel_names = (String[])configurations.get("OutputChannels");
        final int width_out = in.getSizeX();
        final int height_out = in.getSizeY();
        final int t_length_out = in.getSizeT();
        final int channel_scale_out = channel_names.length;
        final int channels_out = in.getSizeC()*channel_scale_out; 
        final int z_length_out = (int) configurations.get("OutputLength");
        final int depth_out = z_length_out;
        final DataType dataType_out = (DataType) configurations.get("OutputDataType");

        // this plug-in processes each channel of each stack in a separate thread
        ExecutorService service = multiThread ? Executors.newFixedThreadPool(cpu) : Executors.newSingleThreadExecutor();
        ArrayList<Future<?>> futures = new ArrayList<Future<?>>(channels * length);

        //Allocate output sequence 
    	for(int t=0; t<t_length_out; ++t) {
			for(int z=0; z<z_length_out; ++z) {
				out.setImage(t, z, new IcyBufferedImage(width_out, height_out, channels_out, dataType_out));
			}
		}
        //out.getColorModel().setColorMap(c, in.getColorModel().getColorMap(c), true);

        for (int t = 0; t < length; t++)
        {
            final int time = t;
            
            final Object out_Z_C_XY = out.getDataXYCZ(time); //[Z][C][XY]
            
            for (int c = 0; c < channels; c++)
            {
                final int channel = c;
                //set channel name
                for (int co = 0; co < channel_scale_out; co++)
            	{
                	out.setChannelName(channel_scale_out*channel+co, in.getChannelName(c)+ "/"+channel_names[co]);
            	}
                futures.add(service.submit(new Runnable()
                {
                    @Override
                    public void run()
                    {
                        Object in_Z_XY = in.getDataXYZ(time, channel);
                        
                        int errorCount = 0;
                        int off = 0;
                        for (int y = 0; y < height; y++)
                        {
                        	
                            for (int x = 0; x < width; x++, off++)
                            {
                            	
                                double[] lineToInput = new double[depth];
                                
                                if (restrictToROI && rois.size() > 0)
                                {
                                    int nbValues = 0;
                                    
                                    for (int z = 0; z < depth; z++)
                                    {
                                        for (ROI roi : rois)
                                        {
                                            if (roi.contains(x, y, z, time, channel))
                                            {
                                                lineToInput[nbValues++] = Array1DUtil.getValue(((Object[]) in_Z_XY)[z], off, dataType);
                                                break;
                                            }
                                        }
                                    }
                                    
                                    if (nbValues == 0) continue;
                                    
                                    if (nbValues < lineToInput.length) lineToInput = Arrays.copyOf(lineToInput, nbValues);
                                }
                                else
                                {
                                    for (int z = 0; z < depth; z++)
                                        lineToInput[z] = Array1DUtil.getValue(((Object[]) in_Z_XY)[z], off, dataType);
                                }
                                
                                double[] lineToOutput = new double[channel_scale_out*depth_out]; //[C][Z]
                               
                                try
                                {
		                               if(mapFunc.process(lineToInput,lineToOutput))
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
				                                            if (roi.contains(x, y, z, time, channel))
				                                            {
				                                            	
				                                            	Array1DUtil.setValue(((Object[][]) out_Z_C_XY)[z][channel_scale_out*channel+co], off, dataType_out,lineToOutput[nbValues++]);
				                                            	
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
		                                    	for (int co = 0; co < channel_scale_out; co++)
		                                    	{
		                                    		for (int z = 0; z < depth_out; z++)
		                                    			Array1DUtil.setValue(((Object[][]) out_Z_C_XY)[z][channel_scale_out*channel+co], off, dataType_out,lineToOutput[nbValues++]);
		                                    	}
			                                }
		                               }
                                }
                                catch(Exception e)
                                {
                                	
                                	System.err.println("Error when excuting Map Fucntion, t:"+time+" c:"+channel+" x:"+x+" y:"+y);
                                	
                            		e.printStackTrace();
                            		
                            		
                            		if(errorCount++>=error_exit_count){
                            			MessageDialog.showDialog("Stoped because of error! t:"+time+" c:"+channel+" x:"+x+" y:"+y,
                        						MessageDialog.ERROR_MESSAGE);
                            			return;
                            		}
                                		

                                }   
                               // Array1DUtil.doubleArrayToSafeArray(lineToOutput, out_Z_XY, dataType.isSigned());
                                
                            } // x
                        } // y
                    }
                }));
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
        return out;
    }
    
    @Override
    public void declareInput(VarList inputMap)
    {
        inputMap.add("input", input.getVariable());
        inputMap.add("map direction", mapDir.getVariable());
        inputMap.add("map function", mapFuncVar.getVariable());
        inputMap.add("restrict to ROI", restrictToROI.getVariable());
    }
    
    @Override
    public void declareOutput(VarList outputMap)
    {
        outputMap.add("map output", output);
    }
    
}
