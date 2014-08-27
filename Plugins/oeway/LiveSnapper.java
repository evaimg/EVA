package plugins.oeway;

import icy.gui.frame.progress.AnnounceFrame;
import icy.image.IcyBufferedImage;
import icy.sequence.Sequence;
import icy.sequence.SequenceAdapter;
import mmcorej.CMMCore;

import plugins.tprovoost.Microscopy.MicroManagerForIcy.MicroscopePlugin;

import plugins.tprovoost.Microscopy.MicroManager.MicroManager;

import icy.gui.dialog.MessageDialog;
import icy.main.Icy;
import java.util.Calendar;
import plugins.adufour.ezplug.*;


public class LiveSnapper extends MicroscopePlugin {
	/**
	 * refresh rate value from corresponding combobox. If the refresh rate is
	 * higher than the necessary time for capture, will not be considered.
	 * */
	/** Reference to the video */
	private Sequence video = null;

	private String currentSeqName="";

	LiveSnapper_GUI gui;
	boolean						stopFlag;
	
	private CMMCore mCore = MicroManager.getCore();
    @Override
    public void start(){
		gui=new LiveSnapper_GUI();

        // generate the user interface
        gui.createUI();
        
        // show the interface to the user
        gui.showUI();
	}
	/**
	 * 
	 * @param img
	 * @see createVideo()
	 */
	private boolean createVideo() {
		video = new Sequence();
		try
		{
	        Calendar calendar = Calendar.getInstance();
			video.setName(currentSeqName + "__" + calendar.get(Calendar.MONTH) + "_" + calendar.get(Calendar.DAY_OF_MONTH) + "_"
	                + calendar.get(Calendar.YEAR) + "-" + calendar.get(Calendar.HOUR_OF_DAY) + "_"
	                + calendar.get(Calendar.MINUTE) + "_" + calendar.get(Calendar.SECOND));
			video.setAutoUpdateChannelBounds(false);
			// sets listener on the frame in order to remove this plugin
			// from the GUI when the frame is closed
			video.addListener(new SequenceAdapter() {
				@Override
				public void sequenceClosed(Sequence sequence) {
					super.sequenceClosed(sequence);
					stopFlag = true;
				}
			});
			Icy.getMainInterface().addSequence(video);
			
	        new AnnounceFrame("New Sequence created:"+currentSeqName,5);
	        return true;
		}
		catch(Exception e)
		{
			new AnnounceFrame("Error when create new sequence!",20);
			return false;
		}
	}


	
	
    @Override
    public void shutdown()
    {
        super.shutdown();
        try
        {

        	if(gui != null)
        		gui.getUI().close();
        }
        catch (Exception e) {
			//System.err.println(e.toString());
		}
    }
	
	
	
	
	/**
	 * 
	 * 
	 * @author Wei Ouyang
	 * 
	 */
	public  class LiveSnapper_GUI extends EzPlug implements EzStoppable
	{
		@Override
		protected void initialize()
		{
		
		}	

		@Override
	    public String getName()
	    {
	        return "LiveSnapper";
	    }
		@Override
		protected void execute()
		{
			stopFlag = false;
			if(mCore.isSequenceRunning())
			{
				
  				MessageDialog.showDialog("Please close other acquisition section first!",
  						MessageDialog.ERROR_MESSAGE);
  				return;
			}
			showProgressBar(true);
			try
			{
				if(video == null)
					createVideo();
				Icy.getMainInterface().addSequence(video);
				
				IcyBufferedImage img;
				while (!stopFlag)
				{
					try {
			            try
			            {
			            	video.beginUpdate();	
			                final CMMCore core = MicroManager.getCore();
				            img = MicroManager.snapImage();
			            	if(img != null)
			            	{
				            	video.setImage(0, 0, img);
			            	}
			            }
			            catch (Exception e) {

						}
			            finally
			            {
							video.endUpdate();
			            }
	
					} catch (Exception e) {
						e.printStackTrace();
					}
					finally
		            {

		            }
				}
			}
			catch (Exception e3) {
				e3.printStackTrace();
			}
			finally
			{
				removeProgressBar();
			}

		}
		
		@Override
		public void clean()
		{
			// use this method to clean local variables or input streams (if any) to avoid memory leaks

		}
		
		@Override
		public void stopExecution()
		{
			// this method is from the EzStoppable interface
			// if this interface is implemented, a "stop" button is displayed
			// and this method is called when the user hits the "stop" button
			stopFlag = true;

		}

	}

}
