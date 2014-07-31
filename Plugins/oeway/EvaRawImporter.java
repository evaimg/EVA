package plugins.oeway;
/*
 * Sam file importer for PAV Scanning Acoustic Microscope
 * Used by EVA project(http://www.evaimg.org/)
 * 
 *  @author Will Ouyang
 */
import java.awt.Rectangle;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.RandomAccessFile;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import javax.swing.filechooser.FileFilter;
import icy.type.collection.array.ArrayUtil;
import plugins.adufour.ezplug.EzGroup;
import plugins.adufour.ezplug.EzPlug;
import plugins.adufour.ezplug.EzVar;
import plugins.adufour.ezplug.EzVarBoolean;
import plugins.adufour.ezplug.EzVarEnum;
import plugins.adufour.ezplug.EzVarFile;
import plugins.adufour.ezplug.EzVarInteger;
import plugins.adufour.ezplug.EzVarListener;
import plugins.adufour.ezplug.EzVarText;
import loci.common.services.ServiceException;
import loci.formats.gui.ExtensionFileFilter;
import loci.formats.ome.OMEXMLMetadataImpl;
import icy.common.exception.UnsupportedFormatException;
import icy.common.listener.ProgressListener;
import icy.file.FileUtil;
import icy.file.Importer;
import icy.file.Loader;
import icy.gui.viewer.Viewer;
import icy.image.IcyBufferedImage;
import icy.image.IcyBufferedImageUtil;
import icy.imagej.ImageJUtil;
import icy.math.ArrayMath;
import icy.plugin.abstract_.PluginImporter;
import icy.plugin.abstract_.PluginSequenceFileImporter;
import icy.type.DataType;
import icy.type.collection.array.Array1DUtil;
import icy.type.collection.array.Array2DUtil;
import ij.ImagePlus;
import ij.io.FileInfo;
import ij.io.FileOpener;
import ij.measure.Calibration;
import ij.process.ImageProcessor;
import icy.sequence.MetaDataUtil;
import icy.sequence.Sequence;
import icy.system.thread.ThreadUtil;

public class EvaRawImporter extends EzPlug implements Importer,EzVarListener
{


	ImagePlus image;
     int sizeX ;
     int sizeY;
     int sizeC;
     int sizeZ;
     int sizeT;
     int type;
    // only integer signed type allowed in ImageJ is 16 bit signed
     boolean signed16;
     String filePath;
     OMEXMLMetadataImpl meta;
		String[] FileTypes = new String[]{
				"8-bit",
				"16-bit Unsigned",
				"16-bit Signed",
				"32-bit Signed",
				"32-bit Unsigned",
				"32-bit Real",
				"64-bit Real",
				"24-bit RGB",
				"24-bit RGB Planar",
				"24-bit BGR",
				"24-bit Integer",
				"32-bit ARGB",
				"32-bit ABGR",
				"1-bit Bitmap",
		} ;
    EzVarText imageTypeVar = new EzVarText("Image type",FileTypes, false);
    EzVarInteger widthVar = new EzVarInteger("Width");
    EzVarInteger heightVar = new EzVarInteger("Height");
    EzVarInteger offsetVar = new EzVarInteger("Offset to image");
    EzVarInteger numImgVar = new EzVarInteger("Number of images");
    EzVarInteger gapVar = new EzVarInteger("Gap between images");
    
 	EzVarBoolean whiteIsZeroVar = new EzVarBoolean("White is zero",false);
 	EzVarBoolean intelByteOrderVar =  new EzVarBoolean("Little-endian byte order",false);
 	EzVarBoolean openAllFilesVar = new EzVarBoolean("Open all files in folder", false);
     
 	EzVarFile fileVar = new EzVarFile("file","");
 	Sequence seq = new Sequence();
 	
	@Override
	protected void initialize() {
		addEzComponent(fileVar);
		
		EzGroup gp=new EzGroup("");
		
		gp.addEzComponent(imageTypeVar);
		gp.addEzComponent(widthVar);
		gp.addEzComponent(heightVar);
		gp.addEzComponent(offsetVar);
		gp.addEzComponent(numImgVar);
		gp.addEzComponent(gapVar);
		gp.addEzComponent(whiteIsZeroVar);
		gp.addEzComponent(intelByteOrderVar);
		gp.addEzComponent(openAllFilesVar);
		
		addEzComponent(gp);
		
		widthVar.setValue(128);
		heightVar.setValue(128);
		numImgVar.setValue(1);
		
		imageTypeVar.addVarChangeListener(this);
		widthVar.addVarChangeListener(this);
		heightVar.addVarChangeListener(this);
		offsetVar.addVarChangeListener(this);
		numImgVar.addVarChangeListener(this);
		gapVar.addVarChangeListener(this);
		whiteIsZeroVar.addVarChangeListener(this);
		openAllFilesVar.addVarChangeListener(this);
		intelByteOrderVar.addVarChangeListener(this);
		
		fileVar.addVarChangeListener(this);
		
		addSequence(seq);
		
	}
	
	
	public FileInfo getFileInfo(String path) {
		File file = new File(path);
        //long length = file.length();
        
		String fileName =FileUtil.getFileName(path);
		String directory =FileUtil.getDirectory(path);

		FileInfo fi = new FileInfo();
		fi.fileFormat = FileInfo.RAW;
		fi.fileName = fileName;
		fi.directory = directory;
		fi.width = widthVar.getValue();
		fi.height = heightVar.getValue();
		int offset = offsetVar.getValue();
		
		if (offset>2147483647)
			fi.longOffset = offset;
		else
			fi.offset = (int)offset;
		
		fi.nImages = numImgVar.getValue();
		fi.gapBetweenImages = gapVar.getValue();
		fi.intelByteOrder = intelByteOrderVar.getValue();  //little-endian
		fi.whiteIsZero = whiteIsZeroVar.getValue();
		

		String imageType = imageTypeVar.getValue();
		if (imageType.equals("8-bit"))
			fi.fileType = FileInfo.GRAY8;
		else if (imageType.equals("16-bit Signed"))
			fi.fileType = FileInfo.GRAY16_SIGNED;
		else if (imageType.equals("16-bit Unsigned"))
			fi.fileType = FileInfo.GRAY16_UNSIGNED;
		else if (imageType.equals("32-bit Signed"))
			fi.fileType = FileInfo.GRAY32_INT;
		else if (imageType.equals("32-bit Unsigned"))
			fi.fileType = FileInfo.GRAY32_UNSIGNED;
		else if (imageType.equals("32-bit Real"))
			fi.fileType = FileInfo.GRAY32_FLOAT;
		else if (imageType.equals("64-bit Real"))
			fi.fileType = FileInfo.GRAY64_FLOAT;
		else if (imageType.equals("24-bit RGB"))
			fi.fileType = FileInfo.RGB;
		else if (imageType.equals("24-bit RGB Planar"))
			fi.fileType = FileInfo.RGB_PLANAR;
		else if (imageType.equals("24-bit BGR"))
			fi.fileType = FileInfo.BGR;
		else if (imageType.equals("24-bit Integer"))
			fi.fileType = FileInfo.GRAY24_UNSIGNED;
		else if (imageType.equals("32-bit ARGB"))
			fi.fileType = FileInfo.ARGB;
		else if (imageType.equals("32-bit ABGR"))
			fi.fileType = FileInfo.ABGR;
		else if (imageType.equals("1-bit Bitmap"))
			fi.fileType = FileInfo.BITMAP;
		else
			fi.fileType = FileInfo.GRAY8;
		return fi;
	}
	
	public static final int unsignedIntToLong(byte[] b) {
	    int l = 0;
	    l |= b[3] & 0xFF;
	    l <<= 8;
	    l |= b[2] & 0xFF;
	    l <<= 8;
	    l |= b[1] & 0xFF;
	    l <<= 8;
	    l |= b[0] & 0xFF;
	    return l;
	  }
	
	public boolean open(String path, int flags)
			throws UnsupportedFormatException, IOException {

		FileInfo fi = getFileInfo(path);
		if (fi==null) return false;
		FileOpener fo = new FileOpener(fi);
		image = fo.open(false);
		if (image!=null) {			
			final int[] dim = image.getDimensions(true);
	        sizeX = dim[0];
	        sizeY = dim[1];
	        sizeC = dim[2];
	        sizeZ = dim[3]; //exchange Z and T
	        sizeT = dim[4];
	        type = image.getType();
	        // only integer signed type allowed in ImageJ is 16 bit signed
	        signed16 = image.getLocalCalibration().isSigned16Bit();
	       
	        meta = MetaDataUtil.createDefaultMetadata(fi.fileName);
	        try {
				MetaDataUtil.setMetaData(meta, sizeX, sizeY, sizeC, sizeZ, sizeT, DataType.UINT,false);
			} catch (ServiceException e) {
				e.printStackTrace();
			}
	        filePath = path;
			return true;
		}
		else
		{
			filePath ="";
			return false;
		}
	}



	@Override
	public boolean load() throws Exception {
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	public void clean() {
		// TODO Auto-generated method stub
		
	}

	@Override
	protected void execute() {
		// TODO Auto-generated method stub
		
	}
    private static void calibrateIcySequence(Sequence sequence, Calibration cal)
    {
        if (cal != null)
        {
            if (cal.scaled())
            {
                // TODO : apply unit conversion
                sequence.setPixelSizeX(cal.pixelWidth);
                sequence.setPixelSizeY(cal.pixelHeight);
                sequence.setPixelSizeZ(cal.pixelDepth);
            }

            // TODO : apply unit conversion
            sequence.setTimeInterval(cal.frameInterval);
        }
    }

	 public  Sequence convertToIcySequence(ImagePlus image, ProgressListener progressListener)
	    {
		 	seq.setName(image.getTitle());
	        final Sequence result = seq;
	        final int[] dim = image.getDimensions(true);

	        final int sizeX = dim[0];
	        final int sizeY = dim[1];
	        final int sizeC = dim[2];
	        final int sizeZ = dim[3];
	        final int sizeT = dim[4];
	        final int type = image.getType();
	        // only integer signed type allowed in ImageJ is 16 bit signed
	        final boolean signed16 = image.getLocalCalibration().isSigned16Bit();

	        final int len = sizeZ * sizeT;
	        int position = 0;

	        result.beginUpdate();
	        try
	        {
	            // convert image
	            for (int t = 0; t < sizeT; t++)
	            {
	                for (int z = 0; z < sizeZ; z++)
	                {
	                    if (progressListener != null)
	                        progressListener.notifyProgress(position, len);

	                    image.setPosition(1, z + 1, t + 1);

	                    // separate RGB channel
	                    if ((sizeC == 1) && ((type == ImagePlus.COLOR_256) || (type == ImagePlus.COLOR_RGB)))
	                        result.setImage(t, z, IcyBufferedImage.createFrom(image.getBufferedImage()));
	                    else
	                    {
	                        final ImageProcessor ip = image.getProcessor();
	                        final Object data = ip.getPixels();
	                        final DataType dataType = ArrayUtil.getDataType(data);
	                        final Object[] datas = Array2DUtil.createArray(dataType, sizeC);

	                        // first channel data (get a copy)
	                        datas[0] = data;
	                        // special case of 16 bits signed data --> subtract 32768
	                        if (signed16)
	                            datas[0] = ArrayMath.subtract(datas[0], Double.valueOf(32768));

	                        // others channels data
	                        for (int c = 1; c < sizeC; c++)
	                        {
	                            image.setPosition(c + 1, z + 1, t + 1);
	                            datas[c] = Array1DUtil.copyOf(image.getProcessor().getPixels());
	                            // special case of 16 bits signed data --> subtract 32768
	                            if (signed16)
	                                datas[c] = ArrayMath.subtract(datas, Double.valueOf(32768));
	                        }

	                        // create a single image from all channels
	                        result.setImage(t, z, new IcyBufferedImage(sizeX, sizeY, datas, signed16));

	                        position++;
	                    }
	                }
	            }



	            // calibrate
	            calibrateIcySequence(result, image.getCalibration());
	        }
	        catch(Exception e)
	        {
	        	e.printStackTrace();
	        }
	        finally
	        {
	            result.endUpdate();
	        }

	        return result;
	    }
	@Override
	public void variableChanged(EzVar source, Object newValue) {
        // can take sometime so we do it in background
        ThreadUtil.bgRun(new Runnable()
        {
            @Override
            public void run()
            {
        		try
        		{
        			seq.removeAllImages();
        			open(fileVar.getValue().getAbsolutePath(),0);
        			convertToIcySequence(image,null);
        		}
        		catch(Exception e)
        		{
        			
        		}
            }
        });

		
	}


	
}
