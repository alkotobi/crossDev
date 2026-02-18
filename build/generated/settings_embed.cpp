#include "settings_embed.h"
#include <string>

// Auto-generated from settings.html - do not edit. Chunked for MSVC string limit.

const std::string& getEmbeddedSettingsHtml() {
    static const std::string html = R"SEmB(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Settings - CrossDev Options</title>
  <style>
    *{box-sizing:border-box}
    body{margin:0;font-family:system-ui,-apple-system,'Segoe UI',Roboto,sans-serif;background:#1e1e1e;color:#e0e0e0;min-height:100vh;padding:24px}
    .settings-header{margin-bottom:24px}
    .settings-header h1{font-size:1.5rem;font-weight:600;margin:0 0 4px 0;color:#fff}
    .settings-subtitle{font-size:0.9rem;color:#888;margin:0}
    .settings-tabs{display:flex;gap:4px;margin-bottom:20px}
    .tab{padding:8px 16px;background:#2d2d2d;border:1px solid #444;border-radius:6px;color:#b0b0b0;cursor:pointer;font-size:0.9rem}
    .tab:hover{background:#383838;color:#e0e0e0}
    .tab.active{background:#0d6efd;border-color:#0d6efd;color:#fff}
    .settings-loading,.settings-error,.settings-success{padding:12px 16px;border-radius:6px;margin-bottom:16px}
    .settings-loading{background:#2d2d2d;color:#888}
    .settings-error{background:rgba(220,53,69,0.2);border:1px solid #dc3545;color:#f8a5a5}
    .settings-success{background:rgba(25,135,84,0.2);border:1px solid #198754;color:#86c9a0}
    .form-section{background:#2d2d2d;border-radius:8px;padding:20px;margin-bottom:16px;overflow-y:auto}
    .form-section h2{font-size:1rem;font-weight:600;margin:0 0 16px 0;color:#fff}
    .form-section h2.form-subsection{margin-top:20px;margin-bottom:12px}
    .form-section h2.form-subsection:first-child{margin-top:0}
    .form-group{margin-bottom:16px}
    .form-group:last-child{margin-bottom:0}
    .form-group label{display:block;font-size:0.85rem;font-weight:500;margin-bottom:6px;color:#b0b0b0}
    .input-with-browse{display:flex;gap:8px;align-items:stretch}
    .input-with-browse input{flex:1;min-width:0}
    .browse-btn{flex-shrink:0;padding:8px 14px;background:#444;border:1px solid #555;border-radius:6px;color:#e0e0e0;font-size:0.85rem;cursor:pointer}
    .browse-btn:hover{background:#555;border-color:#666}
    .form-group input,.form-group select,.form-group textarea{width:100%;padding:8px 12px;background:#1e1e1e;border:1px solid #444;border-radius:6px;color:#e0e0e0;font-size:0.9rem;caret-color:#0d6efd;cursor:text}
    .form-group select{cursor:pointer}
    .form-group textarea{resize:vertical;font-family:ui-monospace,monospace}
    .form-group small{display:block;margin-top:4px;font-size:0.75rem;color:#666}
    .settings-raw{margin-bottom:24px}
    .raw-json{width:100%;min-height:320px;padding:16px;background:#1e1e1e;border:1px solid #444;border-radius:8px;color:#e0e0e0;font-family:ui-monospace,monospace;font-size:13px;line-height:1.5;resize:vertical;caret-color:#0d6efd;cursor:text}
    .raw-json:focus{outline:none;border-color:#0d6efd}
    .raw-hint{font-size:0.8rem;color:#666;margin:8px 0 0 0}
    .settings-actions{display:flex;gap:12px}
    .btn-primary,.btn-secondary{padding:10px 20px;border-radius:6px;font-size:0.9rem;font-weight:500;cursor:pointer;border:none}
    .btn-primary{background:#0d6efd;color:#fff}
    .btn-primary:hover:not(:disabled){background:#0b5ed7}
    .btn-primary:disabled{opacity:0.6;cursor:not-allowed}
    .btn-secondary{background:#444;color:#e0e0e0}
    .btn-secondary:hover:not(:disabled){background:#555}
    .btn-secondary:disabled{opacity:0.6;cursor:not-allowed}
    .hidden{display:none!important}
    .js-dialog-overlay{position:fixed;inset:0;background:rgba(0,0,0,0.6);display:flex;align-items:center;justify-content:center;z-index:1000;cursor:default}
    .js-dialog{background:#2d2d2d;border:1px solid #444;border-radius:8px;padding:20px;min-width:320px;max-width:90vw;cursor:default}
    .js-dialog h3{margin:0 0 12px 0;font-size:1rem;color:#fff}
    .js-dialog input{width:100%;padding:8px 12px;background:#1e1e1e;border:1px solid #444;border-radius:6px;color:#e0e0e0;font-size:0.9rem;caret-color:#0d6efd;cursor:text}
    .js-dialog-actions{margin-top:16px;display:flex;gap:8px;justify-content:flex-end}
    .js-dialog-actions button{padding:8px 16px;border-radius:6px;cursor:pointer;font-size:0.9rem;border:none}
    .js-dialog-ok{background:#0d6efd;color:#fff}
    .js-dialog-ok:hover{background:#0b5ed7}
    .js-dialog-cancel{background:#444;color:#e0e0e0}
    .js-dialog-cancel:hover{background:#555}
    body.js-dialog-open,body.js-dialog-open *{cursor:default!important}
    body.js-dialog-open input{cursor:text!important}
  </style>
</head>
<body>
  <div class="settings-view">
    <header class="settings-header">
      <h1>CrossDev Settings</h1>
      <p class="settings-subtitle">Edit options.json – HTML loading configuration (local, not from server)</p>
    </header>
    <div id="noCrossDev" class="settings-error hidden">CrossDev API not available. This view runs inside the CrossDev desktop app.</div>
    <div id="content" class="hidden">
      <div class="settings-tabs">
        <button type="button" class="tab active" data-tab="form">Form</button>
        <button type="button" class="tab" data-tab="raw">Raw JSON</button>
      </div>
      <div id="loading" class="settings-loading">Loading options...</div>
      <div id="loaded" class="hidden">
        <div id="error" class="settings-error hidden"></div>
        <div id="saveSuccess" class="settings-success hidden">Saved successfully. Restart the app to apply changes.</div>
        <div id="formPanel" class="form-section">
          <div id="formPanelContent"></div>
        </div>
        <div id="rawPanel" class="settings-raw hidden">
          <textarea id="rawJson" class="raw-json" spellcheck="false" placeholder="{}"></textarea>
          <p class="raw-hint">Edit JSON directly. Switch to Form to use structured controls.</p>
        </div>
        <div class="settings-actions">
          <button type="button" class="btn-primary" id="saveBtn">Save</button>
          <button type="button" class="btn-secondary" id="reloadBtn">Reload</button>
        </div>
      </div>
    </div>
  </div>
  <script>
  (function(){
    var options=null,editMode='form',loading=true,saving=false,error='',saveSuccess=false;
    var OPTIONS_SCHEMA={
      htmlLoading:{
        title:'HTML Loading',
        properties:{
          method:{type:'select',enum:['file','url','html'],default:'file',description:'How the main window content is loaded'},
          filePath:{type:'string',browse:'html',showWhen:{method:'file'},placeholder:'e.g. demo.html or dist/index.html',description:'Relative to app working directory or absolute path'},
          url:{type:'string',showWhen:{method:'url'},placeholder:'e.g. http://localhost:5173/',description:'Remote URL to load'},
          htmlContent:{type:'textarea',rows:4,showWhen:{method:'html'},placeholder:'Inline HTML string',description:'Raw HTML content'},
          preloadPath:{type:'string',browse:'js',placeholder:'Path to custom preload script',description:'Leave empty to use built-in bridge'}
        }
      }
    };
    function hasCrossDev(){return typeof window!=='undefined'&&window.CrossDev&&window.CrossDev.invoke;}
    function $id(id){return document.getElementById(id);}
    function show(el){el&&(el.classList.remove('hidden'));}
    function hide(el){el&&(el.classList.add('hidden'));}
    function syncRaw(){try{$id('rawJson').value=JSON.stringify(options||{},null,2);}catch(_){$id('rawJson').value='{}';}}
    function syncFromRaw(){try{options=JSON.parse($id('rawJson').value);error='';return true;}catch(e){error='Invalid JSON: '+(e.message||String(e));return false;}}
    function getAt(obj,path){var p=path.split('.');for(var i=0;i<p.length&&obj;i++)obj=obj[p[i]];return obj;}
    function setAt(obj,path,val){var p=path.split('.');var o=obj;for(var i=0;i<p.length-1;i++){var k=p[i];if(!o[k]||typeof o[k]!=='object')o[k]={};o=o[k];}o[p[p.length-1]]=val;}
    function renderForm(){
      var c=$id('formPanelContent');if(!c)return;c.innerHTML='';
      if(!options||typeof options!=='object'){c.innerHTML='<p class="settings-loading">No options. Add structure )SEmB" R"SEmB(in Raw JSON tab.</p>';return;}
      var schemaKeys=Object.keys(OPTIONS_SCHEMA);
      function renderSection(sectionKey,section,sectionData){
        if(!section.properties)return;
        var h2=document.createElement('h2');h2.className='form-subsection';h2.textContent=section.title||sectionKey;c.appendChild(h2);
        Object.keys(section.properties).forEach(function(propKey){
          var prop=section.properties[propKey];
          var path=sectionKey+'.'+propKey;
          var val=getAt(options,path);
          if(val===undefined)val=prop.default!==undefined?prop.default:'';
          var gr=document.createElement('div');gr.className='form-group';gr.setAttribute('data-path',path);
          if(prop.showWhen){var dep=Object.keys(prop.showWhen)[0];gr.setAttribute('data-show-when',sectionKey+'.'+dep+'='+prop.showWhen[dep]);}
          var lab=document.createElement('label');lab.textContent=propKey.replace(/([A-Z])/g,' $1').replace(/^./,function(s){return s.toUpperCase();});gr.appendChild(lab);
          if(prop.type==='select'){
            var sel=document.createElement('select');sel.setAttribute('data-path',path);
            (prop.enum||[]).forEach(function(opt){var o=document.createElement('option');o.value=opt;o.textContent=opt;sel.appendChild(o);});
            sel.value=String(val||prop.default||'');gr.appendChild(sel);
            sel.addEventListener('change',function(){updateVisibility();});
          }else if(prop.type==='textarea'){
            var ta=document.createElement('textarea');ta.setAttribute('data-path',path);ta.rows=prop.rows||4;ta.value=val||'';ta.placeholder=prop.placeholder||'';gr.appendChild(ta);
          }else if(prop.browse){
            var wrap=document.createElement('div');wrap.className='input-with-browse';
            var inp=document.createElement('input');inp.type='text';inp.setAttribute('data-path',path);inp.value=val||'';inp.placeholder=prop.placeholder||'';wrap.appendChild(inp);
            var btn=document.createElement('button');btn.type='button';btn.className='browse-btn';btn.dataset.filter=prop.browse;btn.textContent='Browse...';wrap.appendChild(btn);
            gr.appendChild(wrap);
          }else{
            var inp=document.createElement('input');inp.type='text';inp.setAttribute('data-path',path);inp.value=val!=null?String(val):'';inp.placeholder=prop.placeholder||'';gr.appendChild(inp);
          }
          if(prop.description){var sm=document.createElement('small');sm.textContent=prop.description;gr.appendChild(sm);}
          c.appendChild(gr);
        });
      }
      schemaKeys.forEach(function(sectionKey){
        var section=OPTIONS_SCHEMA[sectionKey];
        renderSection(sectionKey,section,options[sectionKey]||{});
      });
      var otherKeys=Object.keys(options).filter(function(k){return schemaKeys.indexOf(k)<0;});
      if(otherKeys.length>0){
        var h2=document.createElement('h2');h2.className='form-subsection';h2.textContent='Other';c.appendChild(h2);
        otherKeys.forEach(function(sectionKey){
          var v=options[sectionKey];
          var path=sectionKey;
          var gr=document.createElement('div');gr.className='form-group';gr.setAttribute('data-path',path);
          var lab=document.createElement('label');lab.textContent=sectionKey;gr.appendChild(lab);
          if(v!==null&&typeof v==='object'&&!Array.isArray(v)){
            var ta=document.createElement('textarea');ta.setAttribute('data-path',path);ta.setAttribute('data-json','1');ta.rows=4;ta.value=JSON.stringify(v,null,2);gr.appendChild(ta);
          }else if(Array.isArray(v)){
            var ta=document.createElement('textarea');ta.setAttribute('data-path',path);ta.setAttribute('data-json','1');ta.rows=3;ta.value=JSON.stringify(v);gr.appendChild(ta);
          }else{
            var inp=document.createElement('input');inp.type='text';inp.setAttribute('data-path',path);inp.value=v!=null?String(v):'';gr.appendChild(inp);
          }
          c.appendChild(gr);
        });
      }
      updateVisibility();
      c.querySelectorAll('.browse-btn').forEach(function(btn){
        btn.addEventListener('click',function(){var wrap=btn.closest('.input-with-browse');var inp=wrap?wrap.querySelector('input'):null;browseFile(btn.dataset.filter,'',inp);});
      });
    }
    function updateVisibility(){
      var c=$id('formPanelContent');if(!c)return;
      c.querySelectorAll('[data-show-when]').forEach(function(gr){
        var m=gr.getAttribute('data-show-when');if(!m)return;
        var eqIdx=m.indexOf('=');var depPath=m.substring(0,eqIdx),depVal=m.substring(eqIdx+1);
        var depEl=c.querySelector('input[data-path="'+depPath+'"], select[data-path="'+depPath+'"], textarea[data-path="'+depPath+'"]');
        var show=depEl&&(String(depEl.value||''))===String(depVal);
        gr.classList.toggle('hidden',!show);
      });
    }
    function collectFormData(){
      if(!options)options={};
      var result=JSON.parse(JSON.stringify(options));
      var c=$id('formPanelContent');if(!c)return;
      c.querySelectorAll('input[data-path], select[data-path], textarea[data-path]').forEach(function(el){
        var path=el.dataset.path;if(!path)return;
        var val;
        if(el.dataset.json==='1'){try{val=JSON.parse(el.value||'null');}catch(_){val=el.value;}
        }else if(el.tagName==='SELECT')val=el.value;
        else if(el.type==='checkbox')val=el.checked;
        else val=el.value;
        setAt(result,path,val);
      });
      options=result;
    }
    function loadOptions(){
      if(!hasCrossDev()){show($id('noCrossDev'));$id('loading').textContent='CrossDev not available';return;}
      show($id('content'));loading=true;hide($id('loaded'));show($id('loading'));$id('loading').textContent='Loading options...';
      window.CrossDev.invoke('readOptions',{}).then(function(r){
        loading=false;hide($id('loading'));show($id('loaded'));
        if(r&&r.success&&r.options){options=r.options;renderForm();syncRaw();hide($id('error'));}
        else{$id('error').textContent=r&&r.error?r.error:'Failed to load';show($id('error'));}
      }).catch(function(e){
        loading=false;hide($id('loading'));show($id('loaded'));$id('error').textContent=e&&e.message?e.message:String(e);show($id('error'));
      });
    }
    function saveOptions(){
      if(!hasCrossDev())return;
      if(editMode==='raw'&&!syncFromRaw()){$id('error').textContent=error;show($id('error'));return;}
      if(editMode==='form')collectFormData();
      saving=true;hide($id('error'));hide($id('saveSuccess'));
      window.CrossDev.invoke('writeOptions',{options:options}).then(function(r){
        saving=false;
        if(r&&r.success){show($id('saveSuccess'));$id('saveBtn').textContent='Save';setTimeout(function(){hide($id('saveSuccess'));},3000);}
        else{$id('error').textContent=r&&r.error?r.error:'Failed to save';show($id('error'));}
      }).catch(function(e){
        saving=false;$id('error').textContent=e&&e.message?e.message:String(e);show($id('error'));
      });
      $id('saveBtn').textContent='Saving...';
    }
    function browseFile(filterKey,path,inputEl){
      var titles={html:'Enter HTML file path',js:'Enter preload script path'};
      var currentVal=inputEl?inputEl.value:'';
      document.body.classList.add('js-dialog-open');
      var overlay=document.createElement('div');overlay.className='js-dialog-overlay';
      var d=document.createElement('div');d.className='js-dialog';
      var h=document.createElement('h3');h.textContent=titles[filterKey]||'Enter path';d.appendChild(h);
      var input=document.createElement('input');input.type='text';input.id='jsDialogInput';input.value=currentVal;input.placeholder='e.g. dist/index.html';d.appendChild(input);
      var acts=document.createElement('div');acts.className='js-dialog-actions';
      var cancel=document.createElement('button');cancel.type='button';cancel.className='js-dialog-cancel';cancel.textContent='Cancel';acts.appendChild(cancel);
      var ok=document.c)SEmB" R"SEmB(reateElement('button');ok.type='button';ok.className='js-dialog-ok';ok.textContent='OK';acts.appendChild(ok);
      d.appendChild(acts);overlay.appendChild(d);document.body.appendChild(overlay);
      input.focus();input.select();
      function close(val){document.body.removeChild(overlay);document.body.classList.remove('js-dialog-open');if(val!==undefined&&inputEl)inputEl.value=val;}
      ok.onclick=function(){close(input.value.trim());};
      cancel.onclick=function(){close();};
      overlay.onclick=function(e){if(e.target===overlay)close();};
      input.onkeydown=function(e){if(e.key==='Enter'){e.preventDefault();close(input.value.trim());}if(e.key==='Escape')close();};
    }
    document.querySelectorAll('.tab').forEach(function(btn){
      btn.addEventListener('click',function(){
        editMode=btn.dataset.tab;
        document.querySelectorAll('.tab').forEach(function(b){b.classList.toggle('active',b===btn);});
        if(editMode==='form'){syncFromRaw();renderForm();}
        else{syncRaw();}
        hide($id('formPanel'));hide($id('rawPanel'));
        if(editMode==='form'){show($id('formPanel'));requestAnimationFrame(function(){updateVisibility();});}
        else show($id('rawPanel'));
      });
    });
    $id('rawJson').addEventListener('blur',function(){if(editMode==='raw')syncFromRaw();});
    $id('saveBtn').addEventListener('click',saveOptions);
    $id('reloadBtn').addEventListener('click',loadOptions);
    if(hasCrossDev()){show($id('content'));loadOptions();}else{
      var attempts=0;
      var t=setInterval(function(){attempts++;
        if(hasCrossDev()){clearInterval(t);show($id('content'));loadOptions();}
        else if(attempts>50){clearInterval(t);show($id('noCrossDev'));}
      },100);
    }
  })();
  </script>
</body>
</html>
)SEmB" ;
    return html;
}
