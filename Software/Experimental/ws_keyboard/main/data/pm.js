var ws = null;
var data_row = 1;
var table = document.getElementById('myTable');

function sendUser(user) {
    console.log(`sendUser data: ${table.rows[user.dataset.row].cells[1].innerText}, row: ${user.dataset.row}`)
    console.log(table.rows[user.dataset.row].cells[0].innerText);

    ws.send(JSON.stringify({
        app: "pm",
        website: table.rows[user.dataset.row].cells[0].innerText,
        type: "user",
        data: table.rows[user.dataset.row].cells[1].innerText
      }));
}

function sendPass(user) {
  console.log(`sendUser data: ${table.rows[user.dataset.row].cells[1].innerText}, row: ${user.dataset.row}`)
  console.log(table.rows[user.dataset.row].cells[0].innerText);
    ws.send(JSON.stringify({
        app: "pm",
        website: table.rows[user.dataset.row].cells[0].innerText,
        type: "pass",
        data: table.rows[user.dataset.row].cells[1].innerText
      }));
}

function addUser(site, user, pass) {
  addQuery(site, user);

  ws.send(JSON.stringify({
    app: "pm",
    website: site,
    type: "addUser",
    username: user,
    password: pass
  }));
}

function deleteUser(site, user, pass) {

  ws.send(JSON.stringify({
    app: "pm",
    website: site,
    type: "deleteUser",
    username: user,
    password: pass
  }));
}



function addQuery(website, username) {
  var table = document.getElementById("myTable");
  var row = table.insertRow(-1);
  var cell1 = row.insertCell(0);
  var cell2 = row.insertCell(1);
  var cell3 = row.insertCell(2);
  var cell4 = row.insertCell(3);

// Add some text to the new cells:
  cell1.innerHTML = `${website}`;
  cell2.innerHTML = `<a onclick="sendUser(this)" data-row=${data_row}>${username}</a>`;
  cell3.innerHTML = `<a onclick="sendPass(this)" data-row=${data_row}>******</a>`;
  cell4.innerHTML = `<button class="deletebtn" onclick="deleteInfo(${data_row})" data-row=${data_row}>delete</button>`
  data_row += 1;
}

// Sorting
function search() {
  // Declare variables
  var input, filter, table, tr, td, i, txtValue;
  input = document.getElementById("myInput");
  filter = input.value.toUpperCase();
  table = document.getElementById("myTable");
  tr = table.getElementsByTagName("tr");

  // Loop through all table rows, and hide those who don't match the search query
  for (i = 0; i < tr.length; i++) {
    td = tr[i].getElementsByTagName("td")[0];
    if (td) {
      txtValue = td.textContent || td.innerText;
      if (txtValue.toUpperCase().indexOf(filter) > -1) {
        tr[i].style.display = "";
      } else {
        tr[i].style.display = "none";
      }
    }
  }
}

function beginSocket() {
  ws = new WebSocket('ws://' + document.location.host + '/ws');

  ws.onmessage = function(e){
    console.log("Server returned: " + e.data);
    var json = JSON.parse(e.data);
    if (json.app == "pm" && json.type == "insertUser") {
      addQuery(json.website, json.data);
    }
  }        
}


function saveForm() {
  var test1 = document.getElementById("website").value;
  var test2 = document.getElementById("username").value;
  var test3 = document.getElementById("pwd").value;
  addUser(test1, test2, test3);

  document.getElementById("myForm").reset();
}

function deleteInfo(line) {
  if(!confirm("Are you sure you want to delete these credentials?")) {
    return false;
  }
  //console.log(line);
  var user = table.rows[line].cells[0].innerText;
  var website = table.rows[line].cells[1].innerText;
  console.log(user);

  deleteUser(user, website, "");
  //location.reload();
}