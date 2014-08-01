package plugins.oeway.viewers;

import icy.gui.util.GuiUtil;

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
import plugins.adufour.vars.util.VarListener;

public class WaveformViewer<V extends Iterable<? extends Number>> extends SwingVarEditor<V>{
	public ChartPanel chartPanel ;
	public JFreeChart chart;
	XYSeriesCollection xyDataset = new XYSeriesCollection();
	public double step = 1;
	public final Var<V> variable;
	private VarListener<V> varListener;
	public WaveformViewer(Var<V> var) {
		super(var);
		variable = var;
		varListener = new VarListener<V>(){

			@Override
			public void valueChanged(Var<V> source, V oldValue, V newValue) {
				updateInterfaceValue();
			}

			@Override
			public void referenceChanged(Var<V> source,
					Var<? extends V> oldReference, Var<? extends V> newReference) {
				updateInterfaceValue();
			}
			
		};
		variable.addListener(varListener);
	}

	@Override
	protected JComponent createEditorComponent() {
		// Chart
		chart = ChartFactory.createXYLineChart(
				"","", "Intensity Value", xyDataset,
				PlotOrientation.VERTICAL, false, true, true);
		chartPanel = new ChartPanel(chart, 1024, 500, 500, 200, 10000, 10000, false, false, true, false, true, true);
		//disable autorange
        chart.getXYPlot().getRangeAxis(0).setAutoRange(true);
        chart.getXYPlot().getDomainAxis(0).setAutoRange(true);	
       
        chart.getXYPlot().setDomainCrosshairPaint(Color.red);
        chart.getXYPlot().setRangeCrosshairPaint(Color.red);
		
        XYLineAndShapeRenderer renderer = (XYLineAndShapeRenderer) chart.getXYPlot().getRenderer();
        renderer.setLegendItemToolTipGenerator(
            new StandardXYSeriesLabelGenerator("Legend {0}"));

		// NOW DO SOME OPTIONAL CUSTOMISATION OF THE CHART...
        chart.setBackgroundPaint(Color.white);

//        final StandardLegend legend = (StandardLegend) chart.getLegend();
  //      legend.setDisplaySeriesShapes(true);
        
        // get a reference to the plot for further customisation...
        final XYPlot plot = chart.getXYPlot();
        
        plot.setBackgroundPaint(Color.white );
        
    //    plot.setAxisOffset(new Spacer(Spacer.ABSOLUTE, 5.0, 5.0, 5.0, 5.0));

        
        // set the stroke for each series...
        plot.getRenderer().setSeriesStroke(
            0, 
//            new BasicStroke(
//                1.0f, BasicStroke.CAP_ROUND, BasicStroke.JOIN_ROUND, 
//                10.0f, new float[] {10.0f, 1.0f}, 0.0f
//            )
            new BasicStroke(1.0f)
        );

        plot.getRenderer().setSeriesShape(0, null);
        //plot.getRenderer().setSeriesPaint(0, Color.blue);
        
		return chartPanel;
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
			xyDataset.removeAllSeries();
		}
		catch(Exception e)
		{
			
		}
		XYSeries seriesXY = new XYSeries(variable.getName());	
		V arr = variable.getValue();
		int i = 0;
		for(Number a:arr)
		{							
			//System.out.println(i*pixelSize + " , "+ profile.values[c][i]);
			seriesXY.add(i*step, a);
			i++;
		}
		xyDataset.addSeries(seriesXY);
		chart.fireChartChanged();
		
	}

}
