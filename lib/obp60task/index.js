// Add a new register card in web configuration interface
// This is a Java Script! 
(function(){
    const api = window.esp32nmea2k;
    if (!api) return;
    const tabName = "OBP60";
    api.registerListener((id, data) => {
    //    if (!data.testboard) return; //do nothing if we are not active
        let page = api.addTabPage(tabName, "OBP60");
        api.addEl('h3', '', page, 'Screnshot');
        api.addEl('button', '', page, 'Screenshot').addEventListener('click', function (ev) {
	   window.open('/api/user/OBP60Task/screenshot', 'screenshot');
        });

        api.addEl('hr', '', page); // Separator line
        api.addEl('h3', '', page, 'System status');
        
        // Create a display area for the status
        let statArea = api.addEl('textarea', '', page);
        statArea.style.width = '100%';
        statArea.style.height = '300px';
        statArea.style.fontFamily = 'monospace';
        statArea.readOnly = true;

        // Add a Refresh button for the logs
        api.addEl('button', '', page, 'Refresh Status').addEventListener('click', function() {
            // Standard path for system logs on many ESP32 frameworks
            fetch('/api/status') 
                .then(response => response.text())
                .then(text => {
                    statArea.value = text;
                    statArea.scrollTop = logArea.scrollHeight; // Auto-scroll to bottom
                })
                .catch(err => logArea.value = "Error loading status: " + err);
        });

        api.addEl('hr', '', page); // Separator line
        api.addEl('h3', '', page, 'System log');

	// Create a display area for the log
        let logArea = api.addEl('textarea', '', page);
        logArea.style.width = '100%';
        logArea.style.height = '300px';
        logArea.style.fontFamily = 'monospace';
        logArea.readOnly = true;

        // Add a Refresh button for the logs
        api.addEl('button', '', page, 'Refresh Status').addEventListener('click', function() {
            // Standard path for system logs on many ESP32 frameworks
            fetch('/api/status') 
                .then(response => response.text())
                .then(text => {
                    logArea.value = text;
                    logArea.scrollTop = logArea.scrollHeight; // Auto-scroll to bottom
                })
                .catch(err => logArea.value = "Error loading logs: " + err);
        });



    }, api.EVENTS.init);
})();
