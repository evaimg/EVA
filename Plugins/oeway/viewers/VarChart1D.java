package plugins.oeway.viewers;

import plugins.adufour.vars.gui.VarEditor;

import plugins.adufour.vars.lang.Var;
import plugins.adufour.vars.util.VarListener;

/**
 * Class defining a text label for use with EzPlugs.
 * 
 * @author Will Ouyang
 * 
 */

public class VarChart1D<T extends Iterable<? extends Number>>  extends Var<T>
{

	public VarChart1D(String name, T defaultValue) throws NullPointerException {
		super(name, defaultValue);

	}
	
	@Override
    public WaveformViewer<T> createVarEditor()
    {
		final WaveformViewer<T> ed = new WaveformViewer<T>(this);	
		this.addListener(new VarListener<T>(){

		@Override
		public void valueChanged(Var<T> source, T oldValue, T newValue) {
			ed.updateInterfaceValue();
			
		}

		@Override
		public void referenceChanged(Var<T> source,
				Var<? extends T> oldReference, Var<? extends T> newReference) {
			ed.updateInterfaceValue();
			
		}});
        return ed;
        
    }

}
