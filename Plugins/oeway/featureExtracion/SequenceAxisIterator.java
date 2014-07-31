package plugins.oeway.featureExtraction;

import icy.gui.dialog.MessageDialog;
import icy.image.IcyBufferedImage;
import icy.main.Icy;
import icy.sequence.DimensionId;
import icy.sequence.Sequence;
import icy.sequence.SequenceEvent;
import icy.sequence.SequenceListener;
import icy.type.DataType;
import icy.type.collection.array.Array1DUtil;
import icy.type.point.Point5D;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.image.BufferedImage;
import java.util.Iterator;

import plugins.adufour.blocks.lang.Block;
import plugins.adufour.blocks.util.VarList;
import plugins.adufour.ezplug.EzButton;
import plugins.adufour.ezplug.EzPlug;
import plugins.adufour.ezplug.EzVarBoolean;
import plugins.adufour.ezplug.EzVarDoubleArrayNative;
import plugins.adufour.ezplug.EzVarEnum;
import plugins.adufour.ezplug.EzVarInteger;
import plugins.adufour.ezplug.EzVarSequence;
import plugins.adufour.vars.lang.Var;
import plugins.adufour.vars.util.VarListener;

public class SequenceAxisIterator extends EzPlug implements Iterator<double[]>,Block{

	private Sequence seq;
	private DimensionId dir;
	private Point5D.Integer cur;
	private Point5D.Integer len;
	private DataType dt;
	
	private Object dataXY;
	private Object[][] dataXYZT;
	private Object[] dataXYZ;
	private Object[] dataXYC;
	
	private boolean stop = true;
	
	private EzVarInteger xVar = new EzVarInteger("x");
	private EzVarInteger yVar = new EzVarInteger("y");
	private EzVarInteger zVar = new EzVarInteger("z");
	private EzVarInteger tVar = new EzVarInteger("t");
	private EzVarInteger cVar = new EzVarInteger("c");
    private final EzVarEnum<DimensionId> dirVar  = new EzVarEnum<DimensionId>("Extract Along", new DimensionId[]{DimensionId.X,DimensionId.Y,DimensionId.Z,DimensionId.T,DimensionId.C}, DimensionId.Z);
	private EzVarSequence seqVar = new EzVarSequence("Input");
	private EzVarDoubleArrayNative outputVar = new EzVarDoubleArrayNative("Output",null, false);
	private EzVarBoolean exportAllVar = new EzVarBoolean("Export all data",false);
	private EzButton exportBtn;
	Sequence outputSeq = null;
	public SequenceAxisIterator()
	{

	}
	public SequenceAxisIterator(Sequence sequence,DimensionId direction)
	{
		seq = sequence;
		dir = direction;
		cur = new Point5D.Integer();
		len = new Point5D.Integer();
		reset();
		sequence.addListener(new SequenceListener(){
			@Override
			public void sequenceChanged(SequenceEvent sequenceEvent) {
				len.x = seq.getSizeX();
				len.y = seq.getSizeY();
				len.z = seq.getSizeZ();
				len.t = seq.getSizeT();
				len.c = seq.getSizeC();
				dt = seq.getDataType_();
			}

			@Override
			public void sequenceClosed(Sequence sequence) {
				stop = true;
			}
		});

	}
	public void setCursor(Point5D.Integer p)
	{
		cur = p;
	}
	public Sequence getSequence()
	{
		return seq;
	}
	public Point5D.Integer getCursor()
	{
		return cur;
	}
	public void reset()
	{
		cur.x = 0;
		cur.y = 0;
		cur.z = 0;
		cur.t = 0;
		cur.c = 0;
		
		len.x = seq.getSizeX();
		len.y = seq.getSizeY();
		len.z = seq.getSizeZ();
		len.t = seq.getSizeT();
		len.c = seq.getSizeC();
        switch (dir)
        {
	        case X:
	            cur.x = len.x;
	            dataXY = (Object)seq.getDataXY(cur.t, cur.z, cur.c);
	            break;
	        case Y:
	        	cur.y = len.y;
	        	dataXY = (Object)seq.getDataXY(cur.t, cur.z, cur.c);
	            break;
            case T:
            	cur.t = len.t;
            	dataXYZT = (Object[][]) seq.getDataXYZT(cur.c);
	            break;
            case Z:
            	cur.z = len.z;
            	dataXYZ = (Object[])seq.getDataXYZ(cur.t, cur.c);
	            break;
            case C:
            	cur.c = len.c;
            	dataXYC = (Object[])seq.getDataXYC(cur.t, cur.z);
	            break;
            default:
                throw new UnsupportedOperationException("Direction not supported");
        }
        
        dt = seq.getDataType_();
      
        if(len.x*len.y*len.z*len.t*len.c>0)
        	stop = false;
	}
	
	public int IncreaseAxis(DimensionId Axis)
	{
        switch (dir)
        {
	        case X:
	        case Y:
	        	if(Axis == DimensionId.T ||
	        		Axis == DimensionId.Z ||
	        		Axis == DimensionId.C 
	        	)
	        	dataXY = (Object)seq.getDataXY(cur.t, cur.z, cur.c);
	            break;
            case T:
	        	if(Axis == DimensionId.C)
            	dataXYZT = (Object[][]) seq.getDataXYZT(cur.c);
	            break;
            case Z:
	        	if(Axis == DimensionId.T ||
        			Axis == DimensionId.C 
	        	)
            	dataXYZ = (Object[])seq.getDataXYZ(cur.t, cur.c);
	            break;
            case C:
	        	if(Axis == DimensionId.T ||
        			Axis == DimensionId.Z
	        	)
            	dataXYC = (Object[])seq.getDataXYC(cur.t, cur.z);
	            break;
            default:
                throw new UnsupportedOperationException("Direction not supported");
        }
        switch (Axis)
        {
	        case X:
	        	cur.x++;
	        	if(cur.x>=len.x)
	        		cur.x =0;
	        	return cur.x;
	        case Y:
	        	cur.y++;
	        	if(cur.y>=len.y)
	        		cur.y =0;
	        	return cur.y;
            case T:
            	cur.t++;
	        	if(cur.t>=len.t)
	        		cur.t =0;
            	return cur.t;
            case Z:
            	cur.z++;
	        	if(cur.z>=len.z)
	        		cur.z =0;
            	return cur.z;
            case C:
            	cur.c++;
	        	if(cur.c>=len.c)
	        		cur.c =0;
            	return cur.c;
            default:
                throw new UnsupportedOperationException("Direction not supported");
        }
        
	}
	public void moveToNext()
	{
        switch (dir)
        {
	        case X:
	        	if(IncreaseAxis(DimensionId.Y)==0){
	        		if(IncreaseAxis(DimensionId.Z)==0){
	        			if(IncreaseAxis(DimensionId.T)==0){
	        				if(IncreaseAxis(DimensionId.C)==0){
	        					stop = true;
	        				}
	        			}
	        		}
	        	}
	            break;
	        case Y:
	        	if(IncreaseAxis(DimensionId.Y)==0){
	        		if(IncreaseAxis(DimensionId.Z)==0){
	        			if(IncreaseAxis(DimensionId.T)==0){
	        				if(IncreaseAxis(DimensionId.C)==0){
	        					stop = true;
	        				}
	        			}
	        		}
	        	}
	            break;
            case T:
	        	if(IncreaseAxis(DimensionId.X)==0){
	        		if(IncreaseAxis(DimensionId.Y)==0){
	        			if(IncreaseAxis(DimensionId.Z)==0){
	        				if(IncreaseAxis(DimensionId.C)==0){
	        					stop = true;
	        				}
	        			}
	        		}
	        	}
	            break;
            case Z:
	        	if(IncreaseAxis(DimensionId.X)==0){
	        		if(IncreaseAxis(DimensionId.Y)==0){
	        			if(IncreaseAxis(DimensionId.T)==0){
	        				if(IncreaseAxis(DimensionId.C)==0){
	        					stop = true;
	        				}
	        			}
	        		}
	        	}
	            break;
            case C:
	        	if(IncreaseAxis(DimensionId.X)==0){
	        		if(IncreaseAxis(DimensionId.Y)==0){
	        			if(IncreaseAxis(DimensionId.Z)==0){
	        				if(IncreaseAxis(DimensionId.T)==0){
	        					stop = true;
	        				}
	        			}
	        		}
	        	}
	            break;
            default:
                throw new UnsupportedOperationException("Direction not supported");
        }
	}
	
	public void set(double[] input, int offset)
	{
        switch (dir)
        {
	        case X:
	        	for(cur.x=0;cur.x<len.x;cur.x++)
	        		Array1DUtil.setValue(dataXY, cur.y*len.x+cur.x,dt, input[cur.x+offset]);
	            break;
	        case Y:
	        	for(cur.y=0;cur.y<len.y;cur.y++)
	        		Array1DUtil.setValue(dataXY, cur.y*len.x+cur.x,dt,input[cur.y+offset]);
	            break;
            case T:
	        	for(cur.t=0;cur.t<len.t;cur.t++)
	        		Array1DUtil.setValue(dataXYZT[cur.t][cur.z], cur.y*len.x+cur.x, dt, input[cur.t+offset]);
	            break;
            case Z:
	        	for(cur.z=0;cur.z<len.z;cur.z++)
	        		Array1DUtil.setValue(dataXYZ[cur.z], cur.y*len.x+cur.x,dt,input[cur.z+offset]);
	            break;
            case C:
            	for(cur.c=0;cur.c<len.c;cur.c++)
            		Array1DUtil.setValue(dataXYC[cur.c], cur.y*len.x+cur.x,dt, input[cur.c+offset]);
	            break;
            default:
                throw new UnsupportedOperationException("Direction not supported");
        }
	}
	public double[] get()
	{
		double [] output;
        switch (dir)
        {
	        case X:
	        	output = new double[len.x];
	        	for(cur.x=0;cur.x<len.x;cur.x++)
	        		output[cur.x]=Array1DUtil.getValue(dataXY, cur.y*len.x+cur.x,dt);
	            break;
	        case Y:
	        	output = new double[len.y];
	        	for(cur.y=0;cur.y<len.y;cur.y++)
	        		output[cur.y]=Array1DUtil.getValue(dataXY, cur.y*len.x+cur.x,dt);
	            break;
            case T:
            	output = new double[len.t];
	        	for(cur.t=0;cur.t<len.t;cur.t++)
	        		output[cur.t]=Array1DUtil.getValue(dataXYZT[cur.t][cur.z], cur.y*len.x+cur.x,dt);
	            break;
            case Z:
            	output = new double[len.z];
	        	for(cur.z=0;cur.z<len.z;cur.z++)
	        		output[cur.z]=Array1DUtil.getValue(dataXYZ[cur.z], cur.y*len.x+cur.x,dt);
	            break;
            case C:
            	output = new double[len.c];
            	for(cur.c=0;cur.c<len.c;cur.c++)
            		output[cur.c]=Array1DUtil.getValue(dataXYC[cur.c], cur.y*len.x+cur.x,dt);
	            break;
            default:
                throw new UnsupportedOperationException("Direction not supported");
        }
		return output;
	}

	@Override
	public boolean hasNext() {
		return !stop;
	}

	@Override
	public synchronized double[] next() {
		double [] output=get();
        moveToNext();
		return output;
	}
	public synchronized void setNext(double[] input) {
		set(input,0);
        moveToNext();
	}
	public synchronized void setNext(double[] input,int offset) {
		set(input,offset);
        moveToNext();
	}
	@Override
	public void remove() {
		//do nothing
		
	}
	@Override
	public void declareInput(VarList inputMap) {
		inputMap.add(xVar.getVariable());
		inputMap.add(yVar.getVariable());
		inputMap.add(zVar.getVariable());
		inputMap.add(tVar.getVariable());
		inputMap.add(cVar.getVariable());
		inputMap.add(dirVar.getVariable());
		inputMap.add(seqVar.getVariable());
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
		xVar.getVariable().addListener(listener);
		yVar.getVariable().addListener(listener);
		zVar.getVariable().addListener(listener);
		tVar.getVariable().addListener(listener);
		cVar.getVariable().addListener(listener);
		dirVar.getVariable().addListener(listener);
		seqVar.getVariable().addListener(listener);
	}
	@Override
	public void declareOutput(VarList outputMap) {
		outputMap.add(outputVar.getVariable());
	}
	@Override
	public void run() {
		try
		{
			seq = seqVar.getValue();
		
			dir = dirVar.getValue();
			cur = new Point5D.Integer();
			len = new Point5D.Integer();
			reset();
			
			cur.x = xVar.getValue();
			cur.y = yVar.getValue();
			cur.z = zVar.getValue();
			cur.c = cVar.getValue();
			cur.t = tVar.getValue();
			
			outputVar.setValue(get());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			 if (!isHeadLess())
			  {
				 MessageDialog.showDialog("Sequence Axis Iterator -- Error Occured.",
						 MessageDialog.ERROR_MESSAGE);
			  }
		}
		
	}
	@Override
	public void clean() {

		
	}
	@Override
	protected void execute() {
		try
		{
			if(outputSeq == null)
			{
				Sequence outputSeq = new Sequence();
			}
			double[] da = outputVar.getValue();
			Object out = null;
			Array1DUtil.doubleArrayToSafeArray(da, out, true);
			IcyBufferedImage img = new IcyBufferedImage(da.length, 1, out);	
			outputSeq.addImage(img);
			if(!Icy.getMainInterface().isOpened(outputSeq))
			{
				addSequence(outputSeq);
			}
		}
		catch(Exception e)
		{
			e.printStackTrace();
			MessageDialog.showDialog("Error Occured.",
					MessageDialog.ERROR_MESSAGE);
		}
		
	}
	@Override
	protected void initialize() {
		addEzComponent(xVar);
		addEzComponent(yVar);
		addEzComponent(zVar);
		addEzComponent(tVar);
		addEzComponent(cVar);
		addEzComponent(dirVar);
		addEzComponent(seqVar);
		addEzComponent(exportAllVar);
		addEzComponent(outputVar);
		
		
		ActionListener btnListener = new ActionListener(){

			@Override
			public void actionPerformed(ActionEvent e) {
				
				
			}
			
		};
		exportBtn= new EzButton(null, btnListener );
		addEzComponent(exportBtn);
	}

}
