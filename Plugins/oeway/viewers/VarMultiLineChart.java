package plugins.oeway.viewers;

import org.jfree.data.xy.XYSeries;
import org.jfree.data.xy.XYSeriesCollection;

import plugins.adufour.vars.gui.VarEditor;
import plugins.adufour.vars.lang.Var;

/**
 * Class defining a var chart with jfreechart
 * 
 * @author Will Ouyang
 * 
 */

public class VarMultiLineChart extends Var<XYSeriesCollection>
{
    public VarMultiLineChart(String name, XYSeriesCollection defaultValue)
    {
        super(name, XYSeriesCollection.class, defaultValue);
    }
    
    @Override
    public VarEditor<XYSeriesCollection> createVarEditor()
    {
        return new MultiLineChartViewer(this);//VarEditorFactory.getDefaultFactory().createTextField(this);
    }
  
    public void setValue(double[] newValue)
    {
    	
		XYSeriesCollection dataset = new XYSeriesCollection();
		XYSeries serie = new XYSeries("");
		for(int i=0;i<newValue.length;i++)
		{
			serie.add(i, newValue[i]);
		}
		dataset.removeAllSeries();
		dataset.addSeries(serie);
		this.setValue(dataset);
    	
    }
    public void setValue(double[][] newValue)
    {
		XYSeriesCollection dataset = new XYSeriesCollection();
		XYSeries serie = new XYSeries("");
		for(int i=0;i<newValue.length;i++)
		{
			serie.add(newValue[0][i], newValue[1][i]);
		}
		dataset.removeAllSeries();
		dataset.addSeries(serie);
		this.setValue(dataset);
    }
 
}
