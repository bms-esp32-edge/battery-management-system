# State Estimation Algorithms (SOC, SOH, SOP)

## 1. State of Charge (SOC %)
The SOC estimator combines **Coulomb Counting** with **Open Circuit Voltage (OCV)** lookup corrections:

$$SOC(t) = SOC(t_0) + \frac{1}{C_n} \int_{t_0}^{t} I_{pack}(\tau) \cdot \eta(\tau) \, d\tau$$

- **OCV Reset**: When $|I_{pack}| < 0.05\text{A}$ for $> 300\text{s}$, resting cell voltage directly indexes the `data/ocv_table.csv` table.
- **Coulomb Efficiency ($\eta$)**: $\eta = 0.99$ during discharge, $\eta = 0.96$ during charge.

## 2. State of Health (SOH %)
SOH is calculated using dynamic internal resistance tracking ($R_{int}$):

$$R_{int} = \frac{|\Delta V_{cell}|}{|\Delta I_{pack}|}$$

$$SOH(\%) = \frac{R_{eol} - R_{curr}}{R_{eol} - R_{new}} \times 100$$

## 3. State of Power (SOP)
Dynamic maximum charge/discharge current limits are derived from voltage headroom and thermal constraints:

$$I_{max\_discharge} = \min\left(I_{limit}, \frac{V_{cell\_min} - V_{uvp\_trip}}{R_{int}}\right)$$
