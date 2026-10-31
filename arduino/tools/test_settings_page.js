// Test the actual embedded page script with a minimal DOM/network harness.
const fs=require('fs'),vm=require('vm'),assert=require('assert');
const source=fs.readFileSync(__dirname+'/../hebrew_clock/diagnostics.h','utf8').split('<script>')[1].split('</script>')[0];
const elements={};let submit, posted;
function element(id){return elements[id]??=( {value:'',checked:false,disabled:true,textContent:'',replaceChildren(...v){this.options=v},addEventListener(type,cb){submit=cb}} )}
const context={document:{getElementById:element},Option:function(text,value){this.text=text;this.value=value},URLSearchParams,AbortSignal,Date,setTimeout(){},fetch:async(path,options)=>{
 if(options?.method==='POST'){posted=options;return {ok:true,text:async()=> 'Saved and applied.'}}
 return {ok:true,text:async()=>path==='/timezones'?'Firmware default\nAsia/Jerusalem\nEtc/UTC\n':path==='/settings'?'Asia/Jerusalem\n192.168.1.50\n5514\n1\ntoken123\n':'status data'};
}};
(async()=>{vm.runInNewContext(source,context);await new Promise(setImmediate);
assert.equal(element('zone').value,'Asia/Jerusalem');assert.equal(element('port').value,'5514');assert.equal(element('enabled').checked,true);assert.equal(element('save').disabled,false);
element('zone').value='Etc/UTC';element('enabled').checked=false;
await context.update();assert.equal(element('zone').value,'Etc/UTC');
await submit({preventDefault(){}});assert.equal(posted.headers['X-Clock-Token'],'token123');assert.equal(posted.body.get('enabled'),'0');assert.equal(posted.body.get('ip'),'192.168.1.50');assert.equal(posted.body.get('zone'),'Etc/UTC');assert.equal(element('save').disabled,false);
console.log('Settings page: load, unsaved edits during polling, toggle and save payload passed');
})().catch(e=>{console.error(e);process.exit(1)});
