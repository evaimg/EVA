package plugins.oeway.featureExtraction;

import icy.system.IcyHandledException;

import java.util.ArrayList;
import java.util.HashMap;

import javax.script.ScriptException;

import org.python.core.PyObject;


public class FeatureExtractionInPython extends featureExtractionPlugin {
	Processor myProcessor;
	VarPythonScript inputScript;
	String lastScript="";
	HashMap<String,Object> options= null;
	@Override
	public void initialize(HashMap<String,Object> options, ArrayList<Object> optionUI) {
		EzVarPythonScript ezps = new EzVarPythonScript("Script",
	            "from java.lang import Math\n" +
	            "import copy\n" +
	            "from org.python.modules import jarray\n\n\n" +
	            "def process(input, position):\n" +
	            "\t'''\n" +
	            "\tinput: 1d array\n" +
	            "\tposition: 1d array indicate current position, format: [x,y,z,t,c]\n" +
	            "\t'''\n"+
	            "\toutput=copy.deepcopy(input)\n\n" +
	            "\t#do something here\n\n" +
	            "\treturn output");
		inputScript = (VarPythonScript) ezps.getVariable();
		inputScript.engine.put("options", options);
		optionUI.add(ezps);
		this.options = options;
	}
    interface Processor {
        double[] process(double[] input, double[] position);
    }
    public void compile()
    {
      	try
        {
      		inputScript.engine.clear();
      		inputScript.engine.put("options", options);
            inputScript.evaluate();
        }
        catch (ScriptException e)
        {
            throw new IcyHandledException(e.getMessage());
        }
        PyObject resObject =  inputScript.engine.getPythonInterpreter().get("process");
        System.err.println(resObject.getClass());

        myProcessor = (Processor)resObject.__tojava__(Processor.class);
        
    }

	@Override
	public double[] process(double[] input, double[] position) {
		if(input.length==0)
			return input;
		if(!lastScript.equals(inputScript.getValue()))
		{
			
			compile();
			lastScript = inputScript.getValue();
		}
		return myProcessor.process(input,position);
	}

}
