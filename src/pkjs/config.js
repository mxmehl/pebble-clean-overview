module.exports = [
  {
    "type": "heading",
    "defaultValue": "Clean Overview"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Appearance",
        "size": 4
      },
      {
        "type": "toggle",
        "messageKey": "DARK_MODE",
        "label": "Dark mode",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "SHOW_WEEK",
        "label": "Show calendar week",
        "defaultValue": true
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Seconds Display",
        "size": 4
      },
      {
        "type": "select",
        "messageKey": "SECONDS_MODE",
        "label": "Show seconds",
        "defaultValue": 0,
        "options": [
          {"label": "Off", "value": 0},
          {"label": "Always", "value": 1},
          {"label": "On shake", "value": 2}
        ]
      },
      {
        "type": "select",
        "messageKey": "SHAKE_DURATION",
        "label": "Display duration",
        "defaultValue": 5,
        "options": [
          {"label": "3 seconds", "value": 3},
          {"label": "5 seconds", "value": 5},
          {"label": "10 seconds", "value": 10},
          {"label": "20 seconds", "value": 20}
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Bluetooth",
        "size": 4
      },
      {
        "type": "toggle",
        "messageKey": "VIBRATE_ON_DISCONNECT",
        "label": "Vibrate on disconnect",
        "defaultValue": true
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save"
  }
];
