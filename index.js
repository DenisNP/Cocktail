/* eslint-disable no-console */
const express = require('express');

const app = express();
const port = 3000;

let current = '5|255|255|255|0';
app.get('/set', (req, res) => {
    current = req.query.value;
    res.send(`Set to: ${current}`);
});
app.get('/current', (req, res) => {
    const was = current;
    const c = current.split('|');
    if (Number.parseInt(c[c.length - 1], 10) === 1) {
        c[c.length - 1] = '0';
        current = c.join('|');
    }
    res.send(was);
});

app.listen(port, () => console.log(`Listening on port ${port}!`));
