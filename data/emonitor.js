var ref = window.location.href ;
var host = ref.substr(0,ref.lastIndexOf("/"));
var refOnly = ref.substr(0,ref.lastIndexOf("?"));
function langChange(id){window.location=host+"/wait.html"+"?form_id="+id+"&lang=1&t="+Date.now()}
function BaseViewModel(defaults, remoteUrl, mappings) {
  if(mappings === undefined){mappings = {};}
  var self = this;
  self.remoteUrl = remoteUrl;
  ko.mapping.fromJS(defaults, mappings, self);
  self.fetching = ko.observable(false);
}
BaseViewModel.prototype.update = function (after) {
  if(after === undefined){after = function () { };}
  var self = this;
  self.fetching(true);
  $.get(self.remoteUrl+"?t="+Date.now(), function (data) {
    ko.mapping.fromJS(data, self);
  }, 'json').always(function () {
    self.fetching(false);
    after();
  });
};
function StatusViewModel() {
  var self = this;
  BaseViewModel.call(self, {
"st_uptime": null,"st_timing": null,"st_conn": null,"st_heap": null,"st_bck": null,"st_rst": null,
"meter_co2": null,
"pc_01": "0|0","pc_02": "0|0","pc_03": "0|0","pc_04": "0|0","pc_05": " | ",
"temp_health": null,"temp_count": 0,
"ds18_01": "|.","ds18_02": "|.","ds18_03": "|.","ds18_04": "|.","ds18_05": "|.","ds18_06": "|.","ds18_07": "|.","ds18_08": "|.",
"ds18_09": "|.","ds18_10": "|.","ds18_11": "|.","ds18_12": "|.","ds18_13": "|.","ds18_14": "|.","ds18_15": "|.","ds18_16": "|.",
"st_wifi": null,"st_signal": null,"st_ip" : null
  }, host + '/status.json');

    this.getCounter = ko.observable(0);
    this.getCounter.count = function (value) {
        return ko.computed({
            read: function () {
              switch(value) {
case 1:temp = self.pc_01();break;
case 2:temp = self.pc_02();break;
case 3:temp = self.pc_03();break;
case 4:temp = self.pc_04();break;
case 5:temp = self.pc_05();break;
              }return temp.substr(0,temp.lastIndexOf("|"));}
        }, this);
    }.bind(this.getCounter);
    this.getCounter.level = function (value) {
        return ko.computed({
            read: function () {
              switch(value) {
case 1:temp = self.pc_01();break;
case 2:temp = self.pc_02();break;
case 3:temp = self.pc_03();break;
case 4:temp = self.pc_04();break;
case 5:temp = self.pc_05();break;
              }return temp.substr(temp.lastIndexOf("|")+1,10);}
        }, this);
    }.bind(this.getCounter);
    
    this.getTemp = ko.observable(0);
    this.getTemp.id = function (value) {
        return ko.computed({
            read: function () {
              switch(value) {
case 1:temp = self.ds18_01();break;
case 2:temp = self.ds18_02();break;
case 3:temp = self.ds18_03();break;
case 4:temp = self.ds18_04();break;
case 5:temp = self.ds18_05();break;
case 6:temp = self.ds18_06();break;
case 7:temp = self.ds18_07();break;
case 8:temp = self.ds18_08();break;
case 9:temp = self.ds18_09();break;
case 10:temp = self.ds18_10();break;
case 11:temp = self.ds18_11();break;
case 12:temp = self.ds18_12();break;
case 13:temp = self.ds18_13();break;
case 14:temp = self.ds18_14();break;
case 15:temp = self.ds18_15();break;
case 16:temp = self.ds18_16();break;
              }return temp.substr(0,temp.lastIndexOf("|"));}
        }, this);
    }.bind(this.getTemp);
    this.getTemp.value = function (value) {
        return ko.computed({
            read: function () {
              switch(value) {
case 1:temp = self.ds18_01();break;
case 2:temp = self.ds18_02();break;
case 3:temp = self.ds18_03();break;
case 4:temp = self.ds18_04();break;
case 5:temp = self.ds18_05();break;
case 6:temp = self.ds18_06();break;
case 7:temp = self.ds18_07();break;
case 8:temp = self.ds18_08();break;
case 9:temp = self.ds18_09();break;
case 10:temp = self.ds18_10();break;
case 11:temp = self.ds18_11();break;
case 12:temp = self.ds18_12();break;
case 13:temp = self.ds18_13();break;
case 14:temp = self.ds18_14();break;
case 15:temp = self.ds18_15();break;
case 16:temp = self.ds18_16();break;
              }return temp.substr(temp.lastIndexOf("|")+1,10);}
        }, this);
    }.bind(this.getTemp);
    this.getTemp.visible = function (value) {
        return ko.computed({
            read: function () {
        return (self.temp_count()>=value)}
        }, this);
    }.bind(this.getTemp);
    
  ko.pureComputed(function() {
    return self.pc_01().substr(0,self.pc_01().lastIndexOf("|"));
  }, self);
  // Some devired values
  self.getPulseLevel = ko.pureComputed(function() {
    return self.pc_01().substr(self.pc_01().lastIndexOf("|")+1,10);
  }, self);
  self.hasMeterCO2 = ko.pureComputed(function() {
    if((self.meter_co2() != null) && (self.meter_co2() != 10000)) {
    return true;}return false;}, self);
  self.hasAnalog = ko.pureComputed(function() {
    return (self.pc_05() != " | ");}, self);
}
StatusViewModel.prototype = Object.create(BaseViewModel.prototype);
StatusViewModel.prototype.constructor = StatusViewModel;

function factoryReset(){if(confirm(resetText)){window.location.replace("wait.html?reset=1");}}
function restart(){window.location.replace("wait.html?restart=1");}

function EmonitorViewModel() {
  var self = this;
  self.initialised = ko.observable(false);
  self.updating = ko.observable(false);
  self.statusEnabled = ref.includes("status.html");
  self.clockEnabled = ref.includes("clock.html");
  self.indexEnabled = ref.endsWith("index.html");
  self.waitEnabled = ref.includes("wait.html");
  var updateTimer = null;
  var updateTime = 1 * 1000;
  var pendingRequest = false;
  var pendingTimeOut = 0;
  var cycleCount = 0;
  self.status = new StatusViewModel();

  self.start = function () {delay = 1;
    if(self.statusEnabled){delay = 1;updateTime = 300;}
    if(self.clockEnabled){delay = 1;updateTime = 10000;}
    if(self.waitEnabled){delay = 10000;}
    updateTimer = setTimeout(self.update, delay);};

  self.update = function (){
    if (self.updating()) {return;}
    self.updating(true);
    if (null !== updateTimer){
        clearTimeout(updateTimer);
        updateTimer = null;}

    if(self.statusEnabled){
        if(pendingRequest == false){
            pendingRequest = true;
            self.status.update(function () {
                pendingRequest = false;
                pendingTimeOut = 0;})
        }else{
            pendingTimeOut++;
            if(pendingTimeOut>30){pendingRequest=false;}
        }
    }

    if(self.waitEnabled){
        self.status.update(function () {
            window.location.replace("index.html");})
    }

    if(self.clockEnabled){
        self.status.update(function () {
            clockReceived();})
        clockUpdate();
    }
    //console.log("Cycle:"+cycleCount);cycleCount++;
    updateTimer = setTimeout(self.update, updateTime);
    self.updating(false);
    self.initialised(true);
  };
}

function pageloaded(){var emonitor = new EmonitorViewModel();ko.applyBindings(emonitor);emonitor.start();}