package plugins.oeway.viewers;
import java.awt.Container;
import java.awt.GridBagConstraints;
import java.awt.Insets;
import javax.swing.JComponent;
import plugins.adufour.ezplug.EzVar;

import plugins.adufour.vars.gui.VarEditor;
/**
 * Class defining a EzChart with jfreechart
 * 
 * @author Will Ouyang
 * 
 */
public class EzVarXYChart1D extends EzVar<double[][]>
{
    public EzVarXYChart1D(String varName) throws NullPointerException
    {
        this(varName, null, 0, false);
    }

    
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
    private EzVarXYChart1D(String varName, double[][][] defaultValues, int defaultValueIndex, boolean allowUserInput) throws NullPointerException
    {
        super(new VarXYChart(varName, defaultValues == null ? null : defaultValues[defaultValueIndex]), defaultValues, defaultValueIndex, allowUserInput);
    }
    
    @Override
    protected void addTo(Container container)
    {
        GridBagConstraints gbc = new GridBagConstraints();
        
        gbc.insets = new Insets(2, 10, 2, 5);
        gbc.fill = GridBagConstraints.HORIZONTAL;
        
        
        gbc.weightx = 1;
        
        gbc.gridwidth = GridBagConstraints.REMAINDER;
        
        VarEditor<double[][]> ed = getVarEditor();
        ed.setEnabled(true); // activates listeners
        JComponent component = (JComponent) ed.getEditorComponent();
        component.setPreferredSize(ed.getPreferredSize());
        container.add(component, gbc);
    }
}
