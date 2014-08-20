package plugins.oeway.featureExtraction;

import icy.file.FileUtil;
import icy.system.IcyHandledException;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;

import javax.script.ScriptException;

import org.apache.poi.util.IOUtils;
import org.python.core.PyObject;

import plugins.adufour.ezplug.EzVar;
import plugins.adufour.ezplug.EzVarListener;
import plugins.adufour.ezplug.EzVarText;


public class FeatureExtractionInPython extends featureExtractionPlugin {
	Processor myProcessor;
	VarPythonScript inputScript;
	String lastScript="";
	HashMap<String,Object> options= null;
	EzVarText templateVar;
	HashMap<String,String> library;
	public String readFile(String filename)
	{
	   String content = null;
	   File file = new File(filename); //for ex foo.txt
	   try {
	       FileReader reader = new FileReader(file);
	       char[] chars = new char[(int) file.length()];
	       reader.read(chars);
	       content = new String(chars);
	       reader.close();
	   } catch (IOException e) {
	       e.printStackTrace();
	   }
	   return content;
	}
	@Override
	public void initialize(HashMap<String,Object> options, ArrayList<Object> optionUI) {
		final EzVarPythonScript ezps = new EzVarPythonScript("Script",
	            "from java.lang import Math\n" +
	            "import copy\n" +
	            "from org.python.modules import jarray\n\n\n" +
	            "def process(input, position):\n" +
	            "\t'''\n" +
	            "\tinput: 1d array\n" +
	            "\tposition: 1d array indicate current position, format: [x,y,z,t,c]\n" +
	            "\t'''\n\n"+
	            "\toutput=copy.deepcopy(input)\n\n" +
	            "\t#do something here\n\n" +
	            "\treturn output");
		templateVar = new EzVarText("Library", new String[]{}, 0, false);
		
		inputScript = (VarPythonScript) ezps.getVariable();
		inputScript.engine.put("options", options);

		this.options = options;
		
		library= new HashMap<String,String>();
		library.put("default","");
		File folder = new File(FileUtil.getApplicationDirectory()+FileUtil.separator+"scripts");
		if(folder.exists())
		{
			for(File f:folder.listFiles())
			{
				if(f.isFile())
				{
					if(f.getName().toLowerCase().startsWith("eva_") && f.getName().toLowerCase().endsWith(".py"))
						library.put(f.getName().replace("eva_", "").replace(".py", ""),f.getAbsolutePath());
				}
			}

		}
		else
		{
			FileUtil.createDir(folder);
		}
		
		String[] libs = new String[library.size()];
		int i =0;
		for(String s:library.keySet())
		{
			libs[i++] = s;
		}
		templateVar.setDefaultValues(libs, 0, false);

		templateVar.setToolTipText("Save your customized file in 'ICY_ROOT/scripts' folder, "+
				"naming it start with 'eva_' and end with '.py', "+
				"then it will appear in the library");
		
		templateVar.addVarChangeListener(new EzVarListener<String>(){

			@Override
			public void variableChanged(EzVar<String> source, String newValue) {
				try
				{
					if(templateVar.getValue().equals("default"))
					{
						inputScript.setValue(inputScript.getDefaultValue());
					}
					else
						if(library.containsKey(templateVar.getValue()))
					{
						String scriptString = readFile(library.get(templateVar.getValue()));
						inputScript.setValue(scriptString);
					}
				}
				catch(Exception e)
				{	
					
				}
			}
			
		});
		templateVar.setValue("default");
		optionUI.add(templateVar);
		optionUI.add(ezps);
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
