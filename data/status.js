

let formattedDuration;
let fomDurationSetTime;
let fomDurationOpenTime;
let fomDurationCloseTime;
let fomDurationOpenOffset;
let fomDurationCloseOffset;
let totaldur = 0;
let durationPickerMaker;
let interval;
let timeout;
var callback = function(){
  let pickerElement = document.getElementById("currenttime");
  let pickerElementSetTime = document.getElementById("duration-picker");
  let pickerElementOpenTime = document.getElementById("durpickerOpen");
  let pickerElementCloseTime = document.getElementById("durpickerClose");
  let pickerElementOpenOffset = document.getElementById("durpickerOpenOff");
  let pickerElementCloseOffset = document.getElementById("durpickerCloseOff");
  //let receiverLabel = document.getElementById("output_label");
  //document.getElementById( "#datepicker" ).datepicker();
  class LabelWrapperReceiver {
        constructor(labelElement) {
          this.labelElement = labelElement;
        }
  
        setSecondsValue(value) {
          this.labelElement.textContent = value;
        }
  
  }
  formattedDuration = new FormattedDuration(config = {
    hoursUnitString: ":",
    minutesUnitString: ":",
    secondsUnitString: "",
});

  //let labelWrapperReceier = new LabelWrapperReceiver(receiverLabel);
  
  formattedDuration.SetTotalSeconds(1000);
  fomDurationOpenTime = new FormattedDuration(config = {
    hoursUnitString: "h",
    minutesUnitString: "m",
    secondsUnitString: "s",
});
fomDurationCloseTime = new FormattedDuration(config = {
  hoursUnitString: "h",
  minutesUnitString: "m",
  secondsUnitString: "s",
});
fomDurationOpenOff = new FormattedDuration(config = {
  hoursUnitString: "h",
  minutesUnitString: "m",
  secondsUnitString: "s",
});
fomDurationCloseOff = new FormattedDuration(config = {
  hoursUnitString: "h",
  minutesUnitString: "m",
  secondsUnitString: "s",
});
  //durationPickerMaker.AddSecondChangeObserver(labelWrapperReceier);
  //data.endstophigh
  fetch(window.location.origin+"/get/status")
  .then(response =>{setInterval(watchtick, 1000); return response.json()})
  // .then(response => response.json())
  .then(data => {
    formattedDuration.SetTotalSeconds(data.curtime-1);
    document.getElementById("endstophigh").text=data.endstophigh?"On":"Off";
    document.getElementById("endstoplow").text=data.endstoplow?"On":"Off";
    document.getElementById("doorpos").text=data.doorpos;
    document.getElementById("sunpos").text=data.sunpos;
    document.getElementById('status-box').append(data.doorlog);
fomDurationOpenTime.SetTotalSeconds(data.opentime-1);
fomDurationCloseTime.SetTotalSeconds(data.closetime-1);
fomDurationOpenOff.SetTotalSeconds(data.openoffset-1);
fomDurationCloseOff.SetTotalSeconds(data.closeoffset-1);
durationPickerMakerOpenTime.IncrementSeconds();
durationPickerMakerCloseTime.IncrementSeconds();
durationPickerMakerOpenOff.IncrementSeconds();
durationPickerMakerCloseOff.IncrementSeconds();
durationPickerMaker.IncrementSeconds();
document.getElementById("motorspeed").value=data.motorspeed;
    // $('.temp-value').empty().append(`${data.curtime}C`);
    // $('.humid-value').empty().append(`${data.hval}%`);
    // $('.light-value').empty().append(data.light?"ON":"OFF");
    // $('.fan-value').empty().append(data.fan?"ON":"OFF");
    // $('.time-value').empty().append(data.time);
    // $('.status-box').append(data.status);
    // $('.status-box').scrollTop(99999);

    // setTimeout(updateStatus, 1000); 
    
  });
  // $.ajax({
  //   method: "GET",
  //   url: window.location.origin+"/get/status",success: function(data) {
  //     // formattedDuration.SetTotalSeconds(data.curtime);
  //     document.getElementById("#endstophigh")[0].text=data.endstophigh?"On":"Off";
  //     document.getElementById("#endstoplow")[0].text=data.endstoplow?"On":"Off";
  //     document.getElementById("#doorpos")[0].text=data.doorpos;
  //     document.getElementById("#sunpos")[0].text=data.sunpos;
  //     document.getElementById('.status-box').append(data.doorlog);

  //     // $('.temp-value').empty().append(`${data.curtime}C`);
  //     // $('.humid-value').empty().append(`${data.hval}%`);
  //     // $('.light-value').empty().append(data.light?"ON":"OFF");
  //     // $('.fan-value').empty().append(data.fan?"ON":"OFF");
  //     // $('.time-value').empty().append(data.time);
  //     // $('.status-box').append(data.status);
  //     // $('.status-box').scrollTop(99999);
  //   },
  //   complete: function(obj, status){
  //     // setTimeout(updateStatus, 1000); 
  //     setInterval(watchtick, 1000)
  //   }
  // });
  
  durationPickerMaker = new DurationPickerMaker(formattedDuration);
  durationPickerMaker.SetPickerElement(pickerElement, window, document);
  
  fomDurationSetTime = new FormattedDuration(config = {
    hoursUnitString: "h",
    minutesUnitString: "m",
    secondsUnitString: "s",
});
  durationPickerMaker2 = new DurationPickerMaker(fomDurationSetTime);
  durationPickerMaker2.SetPickerElement(pickerElementSetTime, window, document);


  durationPickerMakerOpenTime = new DurationPickerMaker(fomDurationOpenTime);
  durationPickerMakerOpenTime.SetPickerElement(pickerElementOpenTime, window, document);


  durationPickerMakerCloseTime = new DurationPickerMaker(fomDurationCloseTime);
  durationPickerMakerCloseTime.SetPickerElement(pickerElementCloseTime, window, document);


  durationPickerMakerOpenOff = new DurationPickerMaker(fomDurationOpenOff);
  durationPickerMakerOpenOff.SetPickerElement(pickerElementOpenOffset, window, document);


  durationPickerMakerCloseOff = new DurationPickerMaker(fomDurationCloseOff);
  durationPickerMakerCloseOff.SetPickerElement(pickerElementCloseOffset, window, document);
};

if (
  document.readyState === "complete" ||
  (document.readyState !== "loading" && !document.documentElement.doScroll)
) {
callback();
} else {
document.addEventListener("DOMContentLoaded", callback);
}

function manualopen()
{
  const data = { manualopen: 'example' };
  
  var stuff = new URLSearchParams(data);
  fetch(window.location.origin+"/set/config?"+stuff.toString(), {
  method: 'PUT', // or 'PUT'
  headers: {
    'Content-Type': 'application/json',
  },
  body: JSON.stringify(data),
})
.then(response => response.json())
.then(data => {
  console.log('Success:', data);
})
.catch((error) => {
  console.error('Error:', error);
});
}

function manualclose()
{
  const data = { manualclose: 'example' };
  
  var stuff = new URLSearchParams(data);
  fetch(window.location.origin+"/set/config?"+stuff.toString(), {
    method: 'PUT', // or 'PUT'
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify(data),
  })
  .then(response => response.json())
  .then(data => {
    console.log('Success:', data);
  })
  .catch((error) => {
    console.error('Error:', error);
  });
}

function sendToDevice()
{
    var brightness = $("#brightness").val()
    var universe = $("#universe").val()
    var desIp = $("#desiredIP").val()
    var ssid = $("#SSID").val()
    var pw = $("#password").val()
    var artnet = $("#artnet").is(":checked")
    var tobj = {"hue":hue, "brightness":brightness}//, "universe":universe, "desiredip":desIp, "ssid":ssid, "password":pw, "artnet":artnet};
    if(desIp != "")
        tobj["desiredip"] = desIp
    if(universe != "")
        tobj["universe"] = universe
    if(ssid != "")
        tobj["ssid"] = ssid
    if(password != "")
        tobj["password"] = pw
    tobj["artnet"] = artnet?"on":"off"
  $.ajax({
    method: "PUT",
    url: window.location.origin+"/set/config",
    data: tobj
  });
}

function setMotorSpeed(){
  let val = parseInt(document.getElementById("motorspeed").value);
  val = Math.max(Math.min(val, 10000),1000)
  document.getElementById("motorspeed").value = val;
  var data = {setMotorSpeed: val};  
  var stuff = new URLSearchParams(data);
  fetch(window.location.origin+"/set/config?"+stuff.toString(), {
    method: 'PUT', // or 'PUT'
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify(data),
  })
  .then(response => response.json())
  .then(data => {
    console.log('Success:', data);
  })
  .catch((error) => {
    console.error('Error:', error);
  })
}

function watchtick(){
  durationPickerMaker.IncrementSeconds()
  //$("#file").val(formattedDuration.ToTotalSeconds());
}

function setTimeDate()
{
  var curtime = fomDurationSetTime.ToTotalSeconds();
  // var currentDate = document.getElementById( "#datepicker" ).datepicker( "getDate" );
  var data = {setTime: curtime};
  var stuff = new URLSearchParams(data);
  fetch(window.location.origin+"/set/config?"+stuff.toString(), {
    method: 'POST', // or 'PUT'
    headers: {
      'Content-Type': 'application/x-www-form-url-encoded',
    },
    body: new URLSearchParams(data)//JSON.stringify(data),
  })
  .then(response => response.json())
  .then(data => {
    console.log('Success:', data);
  })
  .catch((error) => {
    console.error('Error:', error);
  })
}

function setDoorTimes()
{
  var open = fomDurationOpenTime.ToTotalSeconds();
  var close = fomDurationCloseTime.ToTotalSeconds();
  var dosunmeme = document.getElementById("sunpositioncheck").val;
  var data = {setOpenTime: open, setCloseTime: close};  
  var stuff = new URLSearchParams(data);
  fetch(window.location.origin+"/set/config?"+stuff.toString(), {
    method: 'PUT', // or 'PUT'
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify(data),
  })
  .then(response => response.json())
  .then(data => {
    console.log('Success:', data);
  })
  .catch((error) => {
    console.error('Error:', error);
  })
}

function setDoorTimeOffsets()
{
  var open = fomDurationOpenOffset.ToTotalSeconds();
  var close = fomDurationCloseOffset.ToTotalSeconds();
  var data = {setOpenOffset: open, setCloseOffset: close};  
  var stuff = new URLSearchParams(data);
  fetch(window.location.origin+"/set/config?"+stuff.toString(), {
    method: 'PUT', // or 'PUT'
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify(data),
  })
  .then(response => response.json())
  .then(data => {
    console.log('Success:', data);
  })
  .catch((error) => {
    console.error('Error:', error);
  })
}

function startcure()
{
  var opt = document.getElementById( "#animListId option:selected" ).text();
  var dat = {"startcure":"wow!"};
  var dur = formattedDuration.ToTotalSeconds();
  dat["duration"] = dur;
  $.ajax({
    method: "PUT",
    url: window.location.origin+"/set/config",
    data: dat
  });
  cleanup()
  clearTimeout(timeout)
  $("#file").prop("max",formattedDuration.ToTotalSeconds());

  interval = setInterval(watchtick, 1000)
  timeout = setTimeout(cleanup, formattedDuration.ToTotalSeconds()*1000+1000);
 
  $(".progress").show();
  $(".cure-button").prop('disabled', true);
}


function toggleon()
{
  var opt = $( "#animListId option:selected" ).text();
  var dat = {"toggleled":"wow!"};
  $.ajax({
    method: "PUT",
    url: window.location.origin+"/set/config",
    data: dat
  });
}



function demoStep()
{
  var opt = document.getElementById( "#animListId option:selected" ).text();
  var dat = {"demosteps":"wow!"};
  $.ajax({
    method: "PUT",
    url: window.location.origin+"/set/config",
    data: dat
  });
}