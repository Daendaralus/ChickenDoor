

let formattedDuration;
let totaldur = 0;
let durationPickerMaker;
let interval;
let timeout;
$(document).ready(function() {
  let pickerElement = document.getElementById("html-duration-picker");
  //let receiverLabel = document.getElementById("output_label");
  
  class LabelWrapperReceiver {
        constructor(labelElement) {
          this.labelElement = labelElement;
        }
  
        setSecondsValue(value) {
          this.labelElement.textContent = value;
        }
  
  }
  formattedDuration = new FormattedDuration(config = {
    hoursUnitString: " h ",
    minutesUnitString: " m ",
    secondsUnitString: " s ",
});

  //let labelWrapperReceier = new LabelWrapperReceiver(receiverLabel);
  

  durationPickerMaker = new DurationPickerMaker(formattedDuration);
  
  //durationPickerMaker.AddSecondChangeObserver(labelWrapperReceier);
  durationPickerMaker.SetPickerElement(pickerElement, window, document);
  $(".progress").hide()
});

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

function cleanup(){
  clearInterval(interval);
  $(".progress").hide()
  $(".cure-button").prop('disabled', false);
}

function watchtick(){
  durationPickerMaker.DecrementSeconds()
  $("#file").val(formattedDuration.ToTotalSeconds());
}

function startcure()
{
  var opt = $( "#animListId option:selected" ).text();
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



function toggletable()
{
  var opt = $( "#animListId option:selected" ).text();
  var dat = {"toggletable":"wow!"};
  $.ajax({
    method: "PUT",
    url: window.location.origin+"/set/config",
    data: dat
  });
}