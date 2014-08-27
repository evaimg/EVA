package plugins.oeway.viewers;

import java.awt.BasicStroke;
import java.awt.Color;

import javax.swing.JComponent;

import org.jfree.chart.ChartFactory;
import org.jfree.chart.ChartPanel;
import org.jfree.chart.JFreeChart;
import org.jfree.chart.labels.StandardXYSeriesLabelGenerator;
import org.jfree.chart.plot.PlotOrientation;
import org.jfree.chart.plot.XYPlot;
import org.jfree.chart.renderer.xy.XYLineAndShapeRenderer;
import org.jfree.data.xy.XYSeries;
import org.jfree.data.xy.XYSeriesCollection;

import plugins.adufour.vars.gui.swing.SwingVarEditor;
import plugins.adufour.vars.lang.Var;

public class XYChartViewer extends SwingVarEditor<double[][]>{
	double step=1.0;
	public XYChartViewer(Var<double[][]> v) {
		super(v);
	}
	@Override
	protected JComponent createEditorComponent() {
		ChartPanel chartPanel ;
		JFreeChart chart;
		XYSeriesCollection xyDataset = new XYSeriesCollection();
		chart = ChartFactory.createXYLineChart(
				variable.getName(),"", "", xyDataset,
				PlotOrientation.VERTICAL, false, true, true);
		chartPanel = new PanningChartPanel(chart, 512, 300, 300, 150, 10000, 10000, false, false, true, false, true, true);	
        chart.getXYPlot().getRangeAxis(0).setAutoRange(true);
        chart.getXYPlot().getDomainAxis(0).setAutoRange(true);	
        chart.getXYPlot().getRangeAxis(0).setAxisLineVisible(true);
        chart.getXYPlot().getDomainAxis(0).setAxisLineVisible(false);
        chart.getXYPlot().setDomainCrosshairPaint(Color.red);
        chart.getXYPlot().setRangeCrosshairPaint(Color.red);
       chart.getXYPlot().setDomainCrosshairVisible(true);
       chart.getXYPlot().setDomainCrosshairLockedOnData(false);
       chart.getXYPlot().setRangeCrosshairVisible(true);
       chart.getXYPlot().setRangeCrosshairLockedOnData(false);
	       
        XYLineAndShapeRenderer renderer = (XYLineAndShapeRenderer) chart.getXYPlot().getRenderer();
        renderer.setLegendItemToolTipGenerator(
            new StandardXYSeriesLabelGenerator("Legend {0}"));

        chart.setBackgroundPaint(new Color(0.0F, 0.0F, 0.0F, 0.0F));

        final XYPlot plot = chart.getXYPlot();
        
        plot.setBackgroundPaint(Color.white );

        plot.getRenderer().setSeriesStroke(
            0, 
            new BasicStroke(1.0f)
        );
        plot.getRenderer().setSeriesShape(0, null);
        plot.getRenderer().setSeriesPaint(0, Color.blue);
		return chartPanel;
	}
	
    @Override
    public ChartPanel getEditorComponent()
    {
        return (ChartPanel) super.getEditorComponent();
    }
    
	@Override
	protected void activateListeners() {

		
	}

	@Override
	protected void deactivateListeners() {

		
	}

	@Override
	protected void updateInterfaceValue() {
		try
		{
			final XYPlot plot =getEditorComponent().getChart().getXYPlot();

			XYSeriesCollection xyDataset = (XYSeriesCollection) plot.getDataset();
			xyDataset.removeAllSeries();
			XYSeries seriesXY = new XYSeries(variable.getName(),false,true);	
			
			double[][] arr = variable.getValue();
			for(int i=0;i<arr[0].length;i++)
			{							
				seriesXY.add(arr[0][i],arr[1][i]);
			}
			xyDataset.addSeries(seriesXY);
		}
		catch(Exception e)
		{
			
		}
	}
    /**
     * Indicates whether and how this component should resize horizontally if the container panel
     * allows resizing. If multiple components in the same panel support resizing, the amount of
     * extra space available will be shared between all components depending on the returned weight
     * (from 0 for no resizing to 1 for maximum resizing).<br/>
     * By default, this value is 1.0 (horizontal resizing is always allowed to fill up the maximum
     * amount of space)
     * 
     * @return a value from 0 (no resize allowed) to 1 (resize as much as possible)
     */
	@Override
    public double getComponentHorizontalResizeFactor()
    {
        return 1.0;
    }
    
    /**
     * Indicates whether and how this component should resize vertically if the container panel
     * allows resizing. If multiple components in the same panel support resizing, the amount of
     * extra space available will be shared between all components depending on the returned weight
     * (from 0 for no resizing to 1 for maximum resizing).<br/>
     * By default, this value is 0.0 (no vertical resizing)
     * 
     * @return a value from 0 (no resize allowed) to 1 (resize as much as possible)
     */
	@Override
    public double getComponentVerticalResizeFactor()
    {
        return 1.0;
    }
}
