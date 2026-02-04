// Add a new register card in web configuration interface
// This is a Java Script! 
(function(){
    const api = window.esp32nmea2k;
    if (!api) return;
    const tabName = "Screen";

    api.registerListener((id, data) => {
        let page = api.addTabPage(tabName, "Screen");

        // --- Screenshot Button ---
        api.addEl('button', '', page, 'Screenshot').addEventListener('click', function (ev) {
            window.open('/api/user/OBP60Task/screenshot', 'screenshot');
        });

        // --- Log Viewer Section ---
        api.addEl('hr', '', page); // Separator line
        api.addEl('h3', '', page, 'System Logs');
        
        // Create a display area for logs
        let logArea = api.addEl('textarea', '', page);
        logArea.style.width = '100%';
        logArea.style.height = '300px';
        logArea.style.fontFamily = 'monospace';
        logArea.readOnly = true;

        // Add a Refresh button for the logs
        api.addEl('button', '', page, 'Refresh Logs').addEventListener('click', function() {
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
