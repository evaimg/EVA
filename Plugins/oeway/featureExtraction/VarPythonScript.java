package plugins.oeway.featureExtraction;

import javax.script.ScriptException;

import plugins.tprovoost.scripteditor.scriptblock.VarScript;
import plugins.tprovoost.scripteditor.scriptinghandlers.ScriptEngine;
import plugins.tprovoost.scripteditor.scriptinghandlers.ScriptEngineHandler;
import plugins.tprovoost.scripteditor.scriptinghandlers.ScriptingHandler;
import plugins.tprovoost.scripteditor.scriptinghandlers.py.PyScriptEngine;

public class VarPythonScript extends VarScript
{
	public PyScriptEngine engine = (PyScriptEngine) ScriptEngineHandler.getEngine("python");

	public VarPythonScript(String name, String defaultValue)
	{
		super(name, defaultValue);
		super.getEditor().dispose();
		setEditor(new VarPythonScriptEditor(this, defaultValue));
	    ScriptingHandler handler = getEditor().getPanelIn().getScriptHandler();
	    engine = (PyScriptEngine) handler.createNewEngine();
	}

	public void evaluate() throws ScriptException
	{
		engine.eval(getValue());
	}
	public void evaluate(String code) throws ScriptException
	{
		engine.eval(code);
	}	
}
