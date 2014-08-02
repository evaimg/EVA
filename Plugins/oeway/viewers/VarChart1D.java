package plugins.oeway.viewers;

import plugins.adufour.vars.gui.VarEditor;
import plugins.adufour.vars.gui.VarEditorFactory;
import plugins.adufour.vars.gui.swing.TextField;

import plugins.adufour.vars.lang.Var;
import plugins.adufour.vars.lang.VarGenericArray;
import plugins.adufour.vars.util.VarListener;

/**
 * Class defining a var chart with jfreechart
 * 
 * @author Will Ouyang
 * 
 */

public class VarChart1D extends VarGenericArray<double[]>
{
    public VarChart1D(String name, double[] defaultValue)
    {
        super(name, double[].class, defaultValue);
    }
    
    @Override
    public VarEditor<double[]> createVarEditor()
    {
        return new WaveformViewer(this);//VarEditorFactory.getDefaultFactory().createTextField(this);
    }
    
    @Override
    public Object parseComponent(String s)
    {
        return Double.parseDouble(s);
    }
    
    @Override
    public double[] getValue(boolean forbidNull)
    {
        // handle the case where the reference is not an array
        
        @SuppressWarnings("rawtypes")
        Var reference = getReference();
        
        if (reference == null) return super.getValue(forbidNull);
        
        Object value = reference.getValue();
        
        if (value == null) return super.getValue(forbidNull);
        
        if (value instanceof Number) return new double[] { ((Number) value).doubleValue() };
        
        return (double[]) value;
    }
}
