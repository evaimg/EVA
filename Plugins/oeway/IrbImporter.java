package plugins.oeway;

import java.awt.Rectangle;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

import javax.swing.filechooser.FileFilter;

import loci.common.services.ServiceException;
import loci.formats.gui.ExtensionFileFilter;
import loci.formats.ome.OMEXMLMetadataImpl;
import icy.common.exception.UnsupportedFormatException;
import icy.file.FileUtil;
import icy.image.IcyBufferedImage;
import icy.plugin.abstract_.PluginSequenceFileImporter;
import icy.type.DataType;
import ij.ImagePlus;
import ij.io.FileInfo;
import ij.io.FileOpener;
import ij.process.ImageProcessor;
import icy.sequence.MetaDataUtil;

public class IrbImporter extends PluginSequenceFileImporter
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
	public FileInfo getFileInfo(String path) {
		File file = new File(path);
        long length = file.length();
        
		String fileName =FileUtil.getFileName(path);
		String directory =FileUtil.getDirectory(path);
		
		String imageType = "16-bit Signed";
		FileInfo fi = new FileInfo();
		fi.fileFormat = FileInfo.RAW;
		fi.fileName = fileName;
		fi.directory = directory;
		fi.width = 1024;
		fi.height = 768;
		int offset = 7424;
		
		if (offset>2147483647)
			fi.longOffset = offset;
		else
			fi.offset = (int)offset;
		
		fi.nImages = (int) ((length-7424)/(13312+1024*768*2))+2;
		fi.gapBetweenImages = 13312;
		fi.intelByteOrder = true;
		fi.whiteIsZero = false;
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

	@Override
	public String getOpened() {
		
		return filePath;
	}

	@Override
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
	        sizeZ = dim[4]; //exchange Z and T
	        sizeT = dim[3];
	        type = image.getType();
	        // only integer signed type allowed in ImageJ is 16 bit signed
	        signed16 = image.getLocalCalibration().isSigned16Bit();
	       
	        meta = MetaDataUtil.createDefaultMetadata("Sample");
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
	public boolean acceptFile(String path) {
		File file = new File(path);
        InputStream in = null;
        try {
            in = new FileInputStream(file);
            int irbHead[]={0xff,0x49,0x52,0x42};//ff,IRB head
            int tempbyte[] = new int[4];
            for(int i=0;i<4;i++)
            	tempbyte[i]=in.read();
            in.close();
            return tempbyte[0]==irbHead[0] && tempbyte[1]==irbHead[1] &&tempbyte[2]==irbHead[2] &&tempbyte[3]==irbHead[3]; 
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
	}

	@Override
	public List<FileFilter> getFileFilters() {
	     final List<FileFilter> result = new ArrayList<FileFilter>();
	     result.add(new ExtensionFileFilter(new String[] {"irb", "IRB"}, "irb images"));
		return result;
	}

	@Override
	public boolean close() throws IOException {
//		if (image!=null) {
//			image.close();
//		}
		return true;
	}

	@Override
	public OMEXMLMetadataImpl getMetaData() throws UnsupportedFormatException,
			IOException {
		return meta;
	}

	@Override
	public IcyBufferedImage getImage(int serie, int resolution,
			Rectangle rectangle, int z, int t, int c)
			throws UnsupportedFormatException, IOException {
		if(image!=null){
			image.setPosition(c + 1, t + 1, z + 1);  //exchange z and t
			// separate RGB channel
			if ((sizeC == 1)
					&& ((type == ImagePlus.COLOR_256) || (type == ImagePlus.COLOR_RGB)))
				return IcyBufferedImage.createFrom(image.getBufferedImage());
			else {
				final ImageProcessor ip = image.getProcessor();
				return new IcyBufferedImage(sizeX, sizeY, ip.getPixels(), signed16);
			}
		}
		return null;
	}

	
	
}
