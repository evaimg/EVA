package plugins.oeway.viewers;
import java.awt.Container;
import java.awt.GridBagConstraints;
import java.awt.Insets;
import java.util.ArrayList;
import java.util.HashMap;


import javax.swing.JComponent;
import javax.swing.JLabel;


import icy.system.thread.ThreadUtil;

import plugins.adufour.ezplug.EzComponent;
import plugins.adufour.ezplug.EzException;
import plugins.adufour.ezplug.EzGroup;

import plugins.adufour.ezplug.EzVarListener;
import plugins.adufour.vars.gui.VarEditor;

import plugins.adufour.vars.lang.Var;

import plugins.adufour.vars.util.VarListener;

public class EzVarChart1D extends EzComponent implements VarListener<double[]>{

	   /**
     * Creates a new integer variable with a given array of possible values
     * 
     * @param varName
     *            the name of the variable (as it will appear on the interface)
     * @param defaultValues
     *            the list of possible values the user may choose from
     * @param allowUserInput
     *            set to true to allow the user to input its own value manually, false otherwise
     * @throws NullPointerException
     *             if the defaultValues parameter is null
     */
    final VarChart1D                              variable;
    
    private JLabel                            jLabelName;
    
    private VarEditor<double[]>                      varEditor;
    
    private final HashMap<EzComponent, double[][]>   visibilityTriggers = new HashMap<EzComponent, double[][]>();
    
    private final ArrayList<EzVarListener<double[]>> listeners          = new ArrayList<EzVarListener<double[]>>();
    
    
    /**
     * Creates a new integer variable with a given array of possible values
     * 
     * @param varName
     *            the name of the variable (as it will appear on the interface)
     * @param defaultValues
     *            the list of possible values the user may choose from
     * @param defaultValueIndex
     *            the index of the default selected value
     * @param allowUserInput
     *            set to true to allow the user to input its own value manually, false otherwise
     * @throws NullPointerException
     *             if the defaultValues parameter is null
     */
    /**
     * Constructs a new variable
     * 
     * @param variable
     *            The variable to attach to this object
     * @param constraint
     *            the constraint to apply on the variable when receiving input, or null if a default
     *            constraint should be applied
     */
    public EzVarChart1D(String name)
    {
    	super(name);
        this.variable = new VarChart1D(name, new ArrayList<Double>());
        
        ThreadUtil.invokeNow(new Runnable()
        {
            public void run()
            {
            	varEditor = new WaveformViewer(variable);
            }
        });
    }

    /**
     * Constructs a new variable
     * 
     * @param variable
     *            The variable to attach to this object
     * @param constraint
     *            the constraint to apply on the variable when receiving input, or null if a default
     *            constraint should be applied
     */
    public EzVarChart1D(final VarChart1D variable)
    {
    	super(variable.getName());
        this.variable = variable;
        
        ThreadUtil.invokeNow(new Runnable()
        {
            public void run()
            {
            	varEditor = new WaveformViewer(variable);
            }
        });
    }

    protected VarEditor<double[]> getVarEditor()
    {
        return varEditor;
    }
	@Override
	public void valueChanged(Var<double[]> source, double[] oldValue,
			double[] newValue) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void referenceChanged(Var<double[]> source,
			Var<? extends double[]> oldReference,
			Var<? extends double[]> newReference) {
		// TODO Auto-generated method stub
		
	}

	@Override
	protected void addTo(Container container) {
    GridBagConstraints gbc = new GridBagConstraints();
        
        gbc.insets = new Insets(2, 10, 2, 5);
        gbc.fill = GridBagConstraints.HORIZONTAL;
        
//        if (variable.isOptional())
//        {
//            gbc.insets.left = 5;
//            JPanel optionPanel = new JPanel(new BorderLayout(0, 0));
//            
//            final JCheckBox option = new JCheckBox("");
//            option.setSelected(isEnabled());
//            option.setFocusable(false);
//            option.setToolTipText("Click here to disable this parameter");
//            option.addActionListener(new ActionListener()
//            {
//                @Override
//                public void actionPerformed(ActionEvent arg0)
//                {
//                    setEnabled(option.isSelected());
//                    if (option.isSelected())
//                    {
//                        option.setToolTipText("Click here to disable this parameter");
//                    }
//                    else
//                    {
//                        option.setToolTipText("Click here to enable this parameter");
//                    }
//                }
//            });
//            optionPanel.add(option, BorderLayout.WEST);
//            optionPanel.add(jLabelName, BorderLayout.CENTER);
//            container.add(optionPanel, gbc);
//            gbc.insets.left = 10;
//        }
//        else container.add(jLabelName, gbc);
        
        gbc.weightx = 1;
        
        gbc.gridwidth = GridBagConstraints.REMAINDER;
        
        VarEditor<double[]> ed = getVarEditor();
        ed.setEnabled(true); // activates listeners
        JComponent component = (JComponent) ed.getEditorComponent();
        component.setPreferredSize(ed.getPreferredSize());
        container.add(component, gbc);
		
	}
	   /**
     * Returns an EzPlug-wide unique identifier for this variable (used to save/load parameters)
     * 
     * @return a String identifier that is unique within the owner plug
     */
    String getID()
    {
        String id = variable.getName();
        
        EzGroup group = getGroup();
        while (group != null)
        {
            id = group.name + "." + id;
//            group = group.getGroup();
        }
        
        return id;
    }
    
    /**
     * Returns the variable value. By default, null is considered a valid value. In order to show an
     * error message (or throw an exception in head-less mode), use the {@link #getValue(boolean)}
     * method instead
     * 
     * @return The variable value
     */
    public double[] getValue()
    {
        return getValue(false);
    }
    
    /**
     * Returns the variable value (or fails if the variable is null).
     * 
     * @param forbidNull
     *            set to true to display an error message (or to throw an exception in head-less
     *            mode)
     * @return the variable value
     * @throws EzException
     *             if the variable value is null and forbidNull is true
     */
    public double[] getValue(boolean forbidNull)
    {
    	
    	ArrayList<Double> a = (ArrayList<Double>) variable.getValue(forbidNull);
    	double[] d =  new double[a.size()];
    	for(int i=0;i<a.size();i++)
    		d[i] =a.get(i);
        return  d;
    }
    
    /**
     * @return The underlying {@link Var} object that contains the value of this EzVar
     */
    public Var<double[]> getVariable()
    {
        return variable;
    }
    
    /**
     * Indicates whether this variable is enabled, and therefore may be used by plug-ins. Note that
     * this method returns a simple flag, and it is up to the accessing plug-ins to behave
     * accordingly (i.e. it is not guaranteed that a disabled variable will not be used by plug-ins)
     * 
     * @return <code>true</code> if this variable is enabled, <code>false</code> otherwise
     */
    public boolean isEnabled()
    {
        return variable.isEnabled();
    }
    
    /**
     * Removes the given listener from the list
     * 
     * @param listener
     *            the listener to remove
     */
    public void removeVarChangeListener(EzVarListener<double[]> listener)
    {
        listeners.remove(listener);
    }
    
    /**
     * Removes all change listeners for this variable
     */
    public void removeAllVarChangeListeners()
    {
        variable.removeListeners();
    }

	@Override
	public void setToolTipText(String text) {
		// TODO Auto-generated method stub
		
	}
	   
    /**
     * Sets whether the variable is enabled, i.e. that it is usable by plug-ins, and accepts
     * modifications via the graphical interface
     * 
     * @param enabled
     *            the enabled state
     */
    public void setEnabled(boolean enabled)
    {
        variable.setEnabled(enabled);
        jLabelName.setEnabled(enabled);
        getVarEditor().setEnabled(enabled);

    }
    
    /**
     * Sets a flag indicating whether this variable should be considered "optional". This flag is
     * typically used to mark a plug-in parameter as optional, allowing plug-ins to react
     * accordingly and save potentially unnecessary computations. Setting a variable as optional
     * will add a check-box next to the variable editor to provide visual feedback
     * 
     * @param optional
     *            <code>true</code> if this parameter is optional, <code>false</code> otherwise
     */
    public void setOptional(boolean optional)
    {
        variable.setOptional(optional);
    }
    
    /**
     * Sets the new value of this variable
     * 
     * @param value
     *            the new value
     * @throws UnsupportedOperationException
     *             thrown if changing the variable value from code is not supported (or not yet
     *             implemented)
     */
    public void setValue(final double[] value) throws UnsupportedOperationException
    {
    	
    	ArrayList<Double> arr = new ArrayList<Double>();

    	for(int i=0;i<value.length;i++)
    		arr.add(value[i]);
    	
        variable.setValue(arr);

    }
    


}
