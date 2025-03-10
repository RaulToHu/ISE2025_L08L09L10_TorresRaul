t <html><head><title>RTC</title></head>
t <script language=JavaScript type="text/javascript" src="xml_http.js"></script>
t <script language=JavaScript type="text/javascript">
# Define URL and refresh timeout
t var formUpdate = new periodicObj("time.cgx", 1000);
t function plotRTCGraph() {
t  hora = document.getElementById("hora_rtc").value;
t  fecha = document.getElementById("fecha_rtc").value;
t }
t var formUpdate = new periodicObj("time.cgx", 1000);
t function periodicUpdateRTC() {
t  if(document.getElementById("RTCChkBox").checked == true) {
t   updateMultiple(formUpdate,plotRTCGraph);
t   rtc_elTime = setTimeout(periodicUpdateRTC, formUpdate.period);
t  }
t  else
t   clearTimeout(rtc_elTime);
t }
t </script></head>
i pg_header.inc
t <h2 align=center><br>Hora Actual</h2>
t <p><font size="2">En esta pestania se muestra la hora configurada por el usuario. Este modulo
t se ha realizado empleando el reloj <b>LSE</b> del periferico <b>RTC</b> de la tarjeta.</p>
t <form action="time.cgi" method="post" name="cgi">
t <input type="hidden" value="rtc" name="pg">
t <table border=0 width=99%><font size="3">
t <tr bgcolor=#8c71e9>
t  <th width=40%> Valor a representar </th>
t  <th width=40%> Dato a representar</th></tr>
# Here begin data setting which is formatted in HTTP_CGI.C module
t <tr><td><img src=pabb.gif>  Hora:  </td>
c h 1 <td><input type=text name=rtc1 id="time_rtc" size=20 maxlength=20 value="%s"></td></tr>
t <tr><td><img src=pabb.gif>  Fecha:  </TD>
c h 2 <td><input type=text name=rtc2 id="fecha_rtc" size=20 maxlength=20 value="%s"></td></tr>
t </font></table>
# Here begin button definitions
t <p align=center>
t <input type=submit name=set value="Send" id="sbm">
t Periodic:<input type="checkbox" id="RTCChkBox" onclick="periodicUpdateRTC()">
t </p></form>
i pg_footer.inc
. End of script must be closed with period.