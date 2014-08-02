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

public class WaveformViewer extends SwingVarEditor<double[]>{
	

	double step=1.0;
	public WaveformViewer(Var<double[]> v) {
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
       
        chart.getXYPlot().setDomainCrosshairPaint(Color.red);
        chart.getXYPlot().setRangeCrosshairPaint(Color.red);
		
        XYLineAndShapeRenderer renderer = (XYLineAndShapeRenderer) chart.getXYPlot().getRenderer();
        renderer.setLegendItemToolTipGenerator(
            new StandardXYSeriesLabelGenerator("Legend {0}"));

        chart.setBackgroundPaint(Color.white);

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
			XYSeries seriesXY = new XYSeries(variable.getName());	
			double[] arr = variable.getValue();
			int i = 0;
			for(double a:arr)
			{							
				seriesXY.add(i*step, a);
				i++;
			}
			xyDataset.addSeries(seriesXY);
		}
		catch(Exception e)
		{
			
		}
	}

}
