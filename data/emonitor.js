// Work out the endpoint to use, for dev you can change to point at a remote ESP
// and run the HTML/JS from file, no need to upload to the ESP to test

//var reference = window.location.hostname;
var reference = window.location.href ;
var hostName = reference.substr(0,reference.lastIndexOf("/"));


// Convert string to number, divide by scale, return result
// as a string with specified precision
function scaleString(string, scale, precision) {
  var tmpval = parseInt(string) / scale;
  return tmpval.toFixed(precision);
}

function BaseViewModel(defaults, remoteUrl, mappings) {
  if(mappings === undefined){
   mappings = {};
  }
  var self = this;
  self.remoteUrl = remoteUrl;

  // Observable properties
  ko.mapping.fromJS(defaults, mappings, self);
  self.fetching = ko.observable(false);
}

BaseViewModel.prototype.update = function (after) {
  if(after === undefined){
   after = function () { };
  }
  var self = this;
  self.fetching(true);
  $.get(self.remoteUrl, function (data) {
    ko.mapping.fromJS(data, self);
  }, 'json').always(function () {
    self.fetching(false);
    after();
  });
};

//---------------------------------------------------------------------------
function StatusViewModel() {
  var self = this;

  BaseViewModel.call(self, {
    "st_uptime": "",
    "st_timing": "",
    "st_conn": "",
    "st_heap": "",
    "st_bck": "",
    "st_rst": "",
    "pc_01": "",
    "pc_02": "",
    "pc_03": "",
    "pc_04": "",
    "pc_05": "",
    "temp_health": "",
    "temp_count": 0,
    "ds18_01": "",
    "ds18_02": "",
    "ds18_03": "",
    "ds18_04": "",
    "ds18_05": "",
    "ds18_06": "",
    "ds18_07": "",
    "ds18_08": "",
    "ds18_09": "",
    "ds18_10": "",
    "ds18_11": "",
    "ds18_12": "",
    "ds18_13": "",
    "ds18_14": "",
    "ds18_15": "",
    "ds18_16": "",
    "st_wifi": "",
    "st_signal": "",
    "st_ip" : ""
  }, hostName + '/status.json');


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

}
StatusViewModel.prototype = Object.create(BaseViewModel.prototype);
StatusViewModel.prototype.constructor = StatusViewModel;

//---------------------------------------------------------------------------
function EmonitorViewModel() {
  var self = this;

  self.initialised = ko.observable(false);
  self.updating = ko.observable(false);

  self.statusEnabled = reference.endsWith("status.html")
  self.indexEnabled = reference.endsWith("index.html")
 
  var updateTimer = null;
  var updateTime = 1 * 1000;
  var reqCounter = 0;
  self.status = new StatusViewModel();

  // -----------------------------------------------------------------------
  // Initialise the app
  // -----------------------------------------------------------------------
  self.start = function () {
    self.updating(true);

    //Stop timer on index page
    if(!self.indexEnabled) {
       updateTimer = setTimeout(self.update, updateTime);
    }
    else
    {
        self.initialised(true);
    }
    
    self.updating(false);
  };

  // -----------------------------------------------------------------------
  // Get the updated state
  // -----------------------------------------------------------------------
  self.update = function () {
    if (self.updating()) {
      return;
    }
    self.updating(true);
    if (null !== updateTimer) {
      clearTimeout(updateTimer);
      updateTimer = null;
    }
    
    if(self.status.fetching() == false) 
    {
    self.status.remoteUrl = hostName + '/status.json' +"?t="+Date.now() ;
    reqCounter = reqCounter +1;
    console.log("Fetching Data");
      self.status.update(function () {})
    }
    console.log("Cycle");

    updateTimer = setTimeout(self.update, updateTime);
    self.updating(false);
    self.initialised(true);    
  };
}

function pageloaded(){
  // Activates knockout.js
  var emonitor = new EmonitorViewModel();
  ko.applyBindings(emonitor);
  emonitor.start();
}