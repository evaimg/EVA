package plugins.oeway.featureExtraction;

import icy.gui.frame.IcyFrame;
import java.awt.BorderLayout;
import javax.swing.JComponent;
import javax.swing.JPanel;
import plugins.tprovoost.scripteditor.gui.ScriptingPanel;
import plugins.tprovoost.scripteditor.scriptblock.VarScriptEditorV3;

public class VarPythonScriptEditor extends VarScriptEditorV3
{
	public VarPythonScriptEditor(VarPythonScript varScript, String defaultValue)
	{
		super(varScript, defaultValue);
	}

	@Override
	protected JComponent createEditorComponent()
	{
		panelIn = new ScriptingPanel("Internal Editor", "Python");
//		panelOut = new ScriptingPanel("External Editor", "Python");
//		panelOut.setBorder(BorderFactory.createEmptyBorder(4, 4, 4, 4));
//
		frame = new IcyFrame("External Editor", true, true, true, true);
//		frame.setContentPane(panelOut);
//		frame.setSize(500, 500);
//		frame.setVisible(true);
//		frame.addToMainDesktopPane();
//		frame.setDefaultCloseOperation(WindowConstants.HIDE_ON_CLOSE);
//		frame.center();
//
//		frameListener = new IcyFrameAdapter()
//		{
//			@Override
//			public void icyFrameClosing(IcyFrameEvent e)
//			{
//				RSyntaxTextArea textArea = panelIn.getTextArea();
//				textArea.setEnabled(true);
//				textArea.setText(panelOut.getTextArea().getText());
//				textArea.repaint();
//			}
//		};

//		// building east component
//		IcyButton buttonEdit = new IcyButton(new IcyIcon("redo.png", 12));
//		buttonEdit.addActionListener(new ActionListener()
//		{
//
//			@Override
//			public void actionPerformed(ActionEvent e)
//			{
//				panelOut.getTextArea().setText(panelIn.getTextArea().getText());
//				panelIn.getTextArea().setEnabled(false);
//				frame.setVisible(true);
//				frame.requestFocus();
//			}
//		});
//		JPanel eastPanel = new JPanel();
//		eastPanel.setLayout(new BoxLayout(eastPanel, BoxLayout.Y_AXIS));
//		eastPanel.add(Box.createVerticalGlue());
//		eastPanel.add(buttonEdit);
//		eastPanel.add(Box.createVerticalGlue());
//		eastPanel.setOpaque(false);

//		// to Return panel
		JPanel toReturn = new JPanel(new BorderLayout());
		toReturn.add(panelIn, BorderLayout.CENTER);
		toReturn.setOpaque(false);
//		toReturn.add(eastPanel, BorderLayout.EAST);

		setComponentResizeable(true);
		panelIn.getTextArea().setCaretPosition(0);
		//panelOut.getTextArea().setCaretPosition(0);
		return toReturn;
	}
	@Override
    public void setText(String newValue)
    {
		panelIn.getTextArea().setText(newValue);
    }

	@Override
    public ScriptingPanel getPanelIn()
    {
		return panelIn;
    }
	@Override
    public ScriptingPanel getPanelOut()
    {
		return panelIn;
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
